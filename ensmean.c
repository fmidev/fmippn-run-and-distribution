# define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <stdint.h>
#include <ctype.h>
#include <hdf5.h>
#include <hdf5_hl.h>

#define DEFSETS 1000
#define UNIT_DBZ 0
#define UNIT_RATE 1

double ZR_A=223.0, ZR_B=1.53;


int get_variable_string(hid_t h5,char *datasetname, char *attrname, char *str);
uint8_t RfromdBZN(uint8_t dBZN);
uint8_t dBZNfromRi(uint16_t Ri, double gain, double offset);

int main(int argc, char *argv[])
{

   /* HDF5 variables */
  hid_t INH5,OUTH5,whatg,whereg,howg,datasetg,datag,datawhatg,attr;
  hid_t setwhatg,setwhereg,sethowg,dataset,plist,space,datatype,memtype,filetype;
  hsize_t dims[2],chunkdims[2]={100,100};
  size_t typesize,arrsize,databytes;
  
  int unit=UNIT_DBZ;
  double RdBZN[256],K=1.0;
  double xscale,yscale,*sumdata,fval,RATE,meanR;
  /* double LL_lon,LL_lat,UL_lon,UL_lat,UR_lon,UR_lat,LR_lon,LR_lat; */
  double gain=1.0,offset=0.0,fnodata=65535.0,undetect=0.0,goffset;
  uint8_t dBZNRi[65536],*countdata;
  long long int longtime;
  long xsize,ysize,N,Ri;
  uint16_t *membdata,*meandata,meanRi,nodata=65535,val;
  long dBZNi;
  uint8_t *dBZdata,dBZN, DBZ_DATA=0, RATE_DATA=0;
  char projdef[200];
  char timestamp[15]={0};
  char HDFoutfile[300];
  char unitstr[20];
  herr_t ret,status;
  int i,FIRST=1,len;
  H5T_class_t class_id;
  char datagname[100];
  char pgmname[100];
  char datasetname[100],validtimestr[50]={0};
  int membI=0,leadtI=0,leadtimes=DEFSETS,members=DEFSETS;
  FILE *PGMF;
  /*   char membstr[5]={0},leadtstr[5]={0}; */



  setbuf(stdout,NULL);
  H5Eset_auto(H5E_DEFAULT,NULL,NULL);

  INH5=H5Fopen(argv[1], H5F_ACC_RDONLY, H5P_DEFAULT);
  datatype=H5T_STD_U16LE;

  H5LTget_attribute_double(INH5,"/meta/configuration","ZR_A",&ZR_A);
  H5LTget_attribute_double(INH5,"/meta/configuration","ZR_B",&ZR_B);
  /* printf("%d\n",RitodBZN(1,1.0)); */


  /* R from dBZN LUT */
  {
    double R=0.0,B,rcW;
    int N;

    B = 0.05/ZR_B;
    rcW = -(3.2 + log10(ZR_A))/ZR_B;  

    RdBZN[0]=0;
    RdBZN[255]=-1e-6;
    for(N=1;N<255;N++) RdBZN[N]=pow(10.0,B*(double)N + rcW);
  }


  for(leadtI=0;;leadtI++) 
  {
     printf("Lead=%02d\nMembers: ",leadtI);
     for(membI=0;membI<members;membI++)
     {
        sprintf(datasetname,"/member-%02d/leadtime-%02d",membI,leadtI);
        if(FIRST) 
	{
	   H5LTget_dataset_info(INH5, datasetname, dims, &class_id, &typesize );
	   /* printf("%ld %ld\n",xsize,ysize); */
	   xsize=(long)dims[1];
           ysize=(long)dims[0];
           arrsize=xsize*ysize;
           databytes=arrsize*typesize;
	   meandata=malloc(databytes);
           memset(meandata,255,databytes);
           dBZdata=malloc(arrsize);
           countdata=malloc(arrsize);
           membdata=malloc(databytes);
           sumdata=malloc(arrsize*sizeof(double));
           H5LTget_attribute_double(INH5,datasetname,"gain",&gain);
           H5LTget_attribute_double(INH5,datasetname,"nodata",&fnodata);
           H5LTget_attribute_double(INH5,datasetname,"offset",&offset);
           goffset=gain/offset;
           len=get_variable_string(INH5,"meta","nowcast_units",unitstr);
	   /*           printf("varstr: %d %s\n",len,unitstr); */
           if(strstr(unitstr,"dBZ")) DBZ_DATA=1;
           if(strstr(unitstr,"rrate")) RATE_DATA=1;

           nodata=(uint16_t)(fnodata+1e-9);
	   for(Ri=0;Ri<=65535;Ri++) dBZNRi[Ri]=dBZNfromRi(Ri,gain,offset);
	   /*           printf("FIRST %d  gain %f, offset %f, DBZ_DATA %d  RATE_DATA %d\n",FIRST,gain,offset,DBZ_DATA,RATE_DATA); */
           FIRST=0;
	}
        ret=H5LTget_attribute_info(INH5, datasetname, "Valid for", dims, &class_id, &typesize);
        if(ret<0) 
        { 
	  if(!leadtI) members=membI; else leadtimes=leadtI;
          break;
	} else
	  { 
             if(class_id == H5T_INTEGER) 
	     {
	        ret=H5LTget_attribute_long_long(INH5, datasetname, "Valid for", &longtime);
		/*
                dateint=longtime/1000000;
                timeint=longtime%1000000;
		*/
                sprintf(timestamp,"%lld",longtime/100);
	     }

	     if(!membI)
	     { 
                memset(sumdata,0,arrsize*sizeof(double));
                memset(countdata,0,arrsize);
	     }
             H5LTread_dataset(INH5,datasetname,datatype,membdata);
             memset(dBZdata,255,arrsize);
             for(N=0;N<arrsize;N++)
	     {
               val=membdata[N];
	       if(val==nodata) continue;
               fval=(double)val;
	       if(RATE_DATA)
	       { 
                  RATE = fval*gain+offset;
                  dBZN=dBZNRi[val];
	       }

	       if(DBZ_DATA)
	       { 
		  dBZNi=2*(fval*gain + offset)+64;
                  dBZN=(uint8_t)dBZNi;
		  if(dBZNi>=255) dBZN=255; 
		  if(dBZNi<0) dBZN=0; 
	       }
               dBZdata[N]=dBZN;

               if(DBZ_DATA) RATE = RdBZN[dBZN];
               
               sumdata[N] += RATE;
               countdata[N]++;
	     }
             printf("%02d ",membI);

             if(!membI)
	     {
                sprintf(pgmname,"membR_%s_%02d.pgm",timestamp,membI);
                PGMF=fopen(pgmname,"w");
                fprintf(PGMF,"P5\n%ld %ld\n65535\n",xsize,ysize);
                swab(membdata,membdata,databytes);
                fwrite(membdata,1,databytes,PGMF);
                fclose(PGMF);
  
                sprintf(pgmname,"memb0_dBZ_%s.pgm",timestamp);
                PGMF=fopen(pgmname,"w");
                fprintf(PGMF,"P5\n%ld %ld\n255\n",xsize,ysize);
                fwrite(dBZdata,1,arrsize,PGMF);
                fclose(PGMF);
	     }
	  }
     }
     printf("\nMean lead=%02d %s %s\n",leadtI,validtimestr,timestamp);

     /* Get deterministic */
     sprintf(datasetname,"/deterministic/leadtime-%02d",leadtI);
     ret=H5LTget_attribute_info(INH5, datasetname, "Valid for", dims, &class_id, &typesize);
     if(ret >= 0) 
     {
        H5LTread_dataset(INH5,datasetname,datatype,membdata);
        memset(dBZdata,255,arrsize);
        for(N=0;N<arrsize;N++)
        {
           if(membdata[N]==nodata) continue;
	   if(RATE_DATA) dBZdata[N]=dBZNRi[membdata[N]];
           else dBZdata[N]=(uint8_t)(2*((double)membdata[N]*gain+offset)+64);
        }
        printf("Deterministic lead=%02d %s %s\n",leadtI,validtimestr,timestamp);
        sprintf(pgmname,"determdBZ_%s.pgm",timestamp);
        PGMF=fopen(pgmname,"w");
        fprintf(PGMF,"P5\n%ld %ld\n255\n",xsize,ysize);
        fwrite(dBZdata,1,arrsize,PGMF);
        fclose(PGMF);
     }

     /* Get unperturbed */
     sprintf(datasetname,"/unperturbed/leadtime-%02d",leadtI);
     ret=H5LTget_attribute_info(INH5, datasetname, "Valid for", dims, &class_id, &typesize);
     if(ret >= 0) 
     {
        H5LTread_dataset(INH5,datasetname,datatype,membdata);
        memset(dBZdata,255,arrsize);
        for(N=0;N<arrsize;N++)
        {
           if(membdata[N]==nodata) continue;
	   if(RATE_DATA) dBZdata[N]=dBZNRi[membdata[N]];
           else dBZdata[N]=(uint8_t)(2*((double)membdata[N]*gain+offset)+64);
        }
        printf("Unperturbed lead=%02d %s %s\n",leadtI,validtimestr,timestamp);
        sprintf(pgmname,"unpertdBZ_%s.pgm",timestamp);
        PGMF=fopen(pgmname,"w");
        fprintf(PGMF,"P5\n%ld %ld\n255\n",xsize,ysize);
        fwrite(dBZdata,1,arrsize,PGMF);
        fclose(PGMF);
     }

     if(leadtimes<DEFSETS) break;


     for(N=0;N<arrsize;N++) 
     {
        meanR = sumdata[N]/(double)countdata[N];
        if(meanR<655.36 && meanR>=0.0) meanRi = (uint16_t)(meanR*100);
        
        meandata[N]=meanRi;
        dBZN = dBZNRi[meanRi];
        dBZdata[N]=dBZN;
     }
     swab(meandata,meandata,databytes);
     sprintf(pgmname,"meanR_%s.pgm",timestamp);
     PGMF=fopen(pgmname,"w");
     fprintf(PGMF,"P5\n%ld %ld\n65535\n",xsize,ysize);
     fwrite(meandata,1,databytes,PGMF);
     fclose(PGMF);

     sprintf(pgmname,"meandBZ_%s.pgm",timestamp);
     PGMF=fopen(pgmname,"w");
     fprintf(PGMF,"P5\n%ld %ld\n255\n",xsize,ysize);
     fwrite(dBZdata,1,arrsize,PGMF);
     fclose(PGMF);
  }

  
  free(membdata);
  free(meandata);
  free(sumdata);
  free(dBZdata);
  H5Fclose(INH5);

  return(0);
}

/*==================================================================================================*/

uint8_t dBZNfromRi(uint16_t Ri, double gain, double offset)
{
    double R,dBZ;
    int16_t dBZI;

    if(!Ri) dBZI=0;
    else
    {
       if(Ri==65535) dBZI=255;
       else
       {
	  R=gain*(double)Ri+offset;
	  R=0.01*(double)Ri;
          dBZ=10.0*log10(ZR_A*pow(R,ZR_B));
          if(dBZ > -32.0) dBZI=(int)(2.0*dBZ)+64; else dBZI=0;
          if(dBZI > 254) dBZI=254;
       }
    }

    /*    printf("%d=%d ",R,dBZI); */
    return((uint8_t)dBZI);
}


double  dBZNtoR(uint8_t dBZN)
{
  double R=0.0,B,rcW;

  if(dBZN)
  {
     B = 0.05/ZR_B;
     rcW = -(3.2 + log10(ZR_A))/ZR_B;  
     R=pow(10.0,B*dBZN + rcW);
  }
  return(R);
}

/*--------------------------------------------------------------------------------------------------*/

int get_variable_string(hid_t h5,char *datasetname, char *attrname, char *str)
{
   hid_t dataset,attr,memtype,space;
   hvl_t  rdata;             /* Pointer to vlen structures */
   char *ptr;
   int ndims,i,len;
   hsize_t dims[1] = {1};
   herr_t status;

   memset(str,0,strlen(str));
   dataset=H5Dopen(h5,datasetname,H5P_DEFAULT);
   if(dataset<0) dataset=H5Gopen(h5,datasetname,H5P_DEFAULT);
   if(dataset<0) return(-1);
   attr = H5Aopen(dataset, attrname, H5P_DEFAULT);

   space = H5Aget_space(attr);
   memtype = H5Tvlen_create(H5T_NATIVE_CHAR);
   status = H5Aread(attr, memtype, &rdata);
   if(status<0) return(-1);
   ptr = rdata.p;
   len = rdata.len;
   for (i=0; i<len; i++) str[i]=ptr[i];

   status = H5Dvlen_reclaim (memtype, space, H5P_DEFAULT, &rdata);
   status = H5Aclose (attr);
   status = H5Dclose (dataset);
   status = H5Sclose (space);
   status = H5Tclose (memtype);
   return(len);
}

   
   


# if 0

     H5LTget_attribute_long(INH5,"where","xsize",&xsize); 
     H5LTget_attribute_long(INH5,"where","ysize",&ysize);
     H5LTget_attribute_double(INH5,"where","xscale",&xscale);          
     H5LTget_attribute_double(INH5,"where","yscale",&yscale);
     H5LTget_attribute_double(INH5,"where","LL_lon",&LL_lon);
     H5LTget_attribute_double(INH5,"where","LL_lat",&LL_lat);
     H5LTget_attribute_double(INH5,"where","UL_lon",&UL_lon);
     H5LTget_attribute_double(INH5,"where","UL_lat",&UL_lat);
     H5LTget_attribute_double(INH5,"where","UR_lon",&UR_lon);
     H5LTget_attribute_double(INH5,"where","UR_lat",&UR_lat);
     H5LTget_attribute_double(INH5,"where","LR_lon",&LR_lon);
     H5LTget_attribute_double(INH5,"where","LR_lat",&LR_lat);
     H5LTget_attribute_string(INH5,"where","projdef",projdef);
     H5Fclose(INH5);
  }

  arrsize=xsize*ysize;
  membsize=arrsize*sizeof(int); 
  dims[0]=(hsize_t)xsize;
  dims[1]=(hsize_t)ysize;
  datatype= H5T_STD_U8LE;







   sprintf(HDFoutfile,"%s/%s_%03d-%03d_PACC.h5",outdir,obstime,Accmins[p],Interval[p]);
    OUTH5 = H5Fcreate(HDFoutfile,H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT);
    H5LTset_attribute_string(OUTH5,"/","Conventions","ODIM_H5/V2_1");

    dataspace=H5Screate_simple(2, dims, NULL);
    plist = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(plist, 2, chunkdims);
    H5Pset_deflate( plist, 6);

    whatg=H5Gcreate2(OUTH5,"what",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
    whereg=H5Gcreate2(OUTH5,"where",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
    howg=H5Gcreate2(OUTH5,"how",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

    H5LTset_attribute_string(OUTH5,"what","date",datadate);
    H5LTset_attribute_string(OUTH5,"what","time",datatime);
    H5LTset_attribute_string(OUTH5,"what","object","COMP");
    H5LTset_attribute_string(OUTH5,"what","source","ORG:EFKL");
    H5LTset_attribute_string(OUTH5,"what","version","H5rad 2.1");

    H5LTset_attribute_double(OUTH5,"where","LL_lon",&LL_lon,1);
    H5LTset_attribute_double(OUTH5,"where","LL_lat",&LL_lat,1);
    H5LTset_attribute_double(OUTH5,"where","UL_lon",&UL_lon,1);
    H5LTset_attribute_double(OUTH5,"where","UL_lat",&UL_lat,1);
    H5LTset_attribute_double(OUTH5,"where","UR_lon",&UR_lon,1);
    H5LTset_attribute_double(OUTH5,"where","UR_lat",&UR_lat,1);
    H5LTset_attribute_double(OUTH5,"where","LR_lon",&LR_lon,1);
    H5LTset_attribute_double(OUTH5,"where","LR_lat",&LR_lat,1);
    H5LTset_attribute_long(OUTH5,"where","xsize",&xsize,1);
    H5LTset_attribute_long(OUTH5,"where","ysize",&ysize,1);
    H5LTset_attribute_double(OUTH5,"where","xscale",&xscale,1);
    H5LTset_attribute_double(OUTH5,"where","yscale",&yscale,1);
    H5LTset_attribute_string(OUTH5,"where","projdef",projdef);

   for(f=0;f<Fields[p];f++)
    {
        /* Define dataset start and end times */
        for(i=0;i<2;i++)
        {
           Forinds[i]=AccInds[i][p][f];
           formins=Forinds[i]*5;
           secs=bsecs+formins*60;
           date_from_sec(fortime,secs);
           sprintf(fielddate[i],"%.8s",fortime);
           sprintf(fieldtime[i],"%.4s00",fortime+8);
        }

        /* HDF5 dataset attributes */
        sprintf(datasetname,"dataset%d",f+1);
        printf("dataset%d\n",f+1);
        datasetg=H5Gcreate2(OUTH5,datasetname,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
        setwhatg=H5Gcreate2(datasetg,"what",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
        H5LTset_attribute_string(datasetg,"what","product","COMP");
        H5LTset_attribute_string(datasetg,"what","quantity","PROB");
        H5LTset_attribute_double(datasetg,"what","gain",&gain,1);
        H5LTset_attribute_double(datasetg,"what","offset",&offset,1);
        H5LTset_attribute_double(datasetg,"what","nodata",&nodata,1);
        H5LTset_attribute_double(datasetg,"what","undetect",&undetect,1);
        H5LTset_attribute_string(datasetg,"what","startdate",fielddate[0]);
        H5LTset_attribute_string(datasetg,"what","starttime",fieldtime[0]);
        H5LTset_attribute_string(datasetg,"what","enddate",fielddate[1]);
        H5LTset_attribute_string(datasetg,"what","endtime",fieldtime[1]);

       /* Looping thru thresholds and writing data to dataset f for each threshold */
        for(thri=1;thri<=Thresholds[p];thri++)
        {
          sprintf(datagname,"data%ld",thri);        
          printf("\tdata%ld\n",thri);        
          datag=H5Gcreate2(datasetg,datagname,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
          datawhatg=H5Gcreate2(datag,"what",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
          H5LTset_attribute_long(datag,"what","threshold_id",&thri,1);
          H5LTset_attribute_double(datag,"what","threshold_value",&f_acclims[p][thri],1);
 
          /* create new empty dataset and attributes to destination file */ 
          dataset = H5Dcreate2(datag,"data", datatype, dataspace,
                    H5P_DEFAULT, plist, H5P_DEFAULT);
          H5LTset_attribute_string( datag, "data", "CLASS", "IMAGE");
          H5LTset_attribute_string( datag, "data", "IMAGE_VERSION", "1.2");
          H5Dwrite(dataset, datatype, H5S_ALL, dataspace, H5P_DEFAULT,&out_fracarr[thri][0]);
          H5Dclose(dataset);
          H5Gclose(datawhatg);
          H5Gclose(datag);
        }
        H5Gclose(setwhatg);
        H5Gclose(datasetg);
    }
    H5Gclose(whatg);
    H5Gclose(whereg);
    H5Gclose(howg);
  }
  H5Fclose(OUTH5);

# endif
