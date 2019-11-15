#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "gd.h"
#include "gdfontl.h"
#include "gdfontmb.h"
#include "gdfonts.h"
#include "gdfontt.h"
 
typedef unsigned long int ulong;
typedef unsigned char uchar;

struct Colors {
                 uchar N;
                 uchar R;
                 uchar G;
                 uchar B;
              };
         

static long int Xdim,Ydim,arrsize;

void insert_legend
    (uchar *maparr, gdImagePtr legend, long int xpos, long int ypos);

void GD_to_ppm(unsigned char *ppmarr, gdImagePtr im);

struct tm utc_to_local(char *utc, long int timediff);

int main(int argc, char *argv[])
{

  gdImagePtr legend;  
  
  static struct Colors col[10];
  struct tm LT;
 
  uchar *inarr,
        *maparr,
	*outarr,
        c,
	r,mr,
	g,mg,
	b,mb; 


  char *argpath,
       *picpath,
       *utc,
       infile[200]={0},
       filename[200]={0},
       mapfile[200]={0},
       outfile[200]={0},
       classfile[200]={0},
       legtext[200]={0},
       sline[200]={0},
       legtime[200]={0},
       *classname,
       intype,maptype,*par;

  long int n,N;

  int i,
      fontcolor,
      bgcolor,
      colind;

  float dBZ;     

  FILE *INFILE,
       *MAPFILE,
       *OUTFILE,
       *CLASSFILE;


   argpath=getenv("SUOMI_ARGPATH");
   picpath=getenv("SUOMI_PIC_PATH");
   classname=getenv("SUOMI_RAINCLASSES");
   
   par=strrchr(argv[1],'/');
   if(par!=NULL) par++; else par=argv[1];
   sprintf(infile,"%s",par);
   printf("%s\n",infile);
   arrsize=Xdim*Ydim;

   sprintf(classfile,"%s",classname);
   CLASSFILE=fopen(classfile,"r");
   fgets(sline,199,CLASSFILE);
   fgets(sline,199,CLASSFILE);
   c=0;
   while(1==1)
   {
      fgets(sline,199,CLASSFILE);
      if(sline[0]=='\n') break;
      sscanf(sline,"%*d %*d %*d %f %u %u %u",
             &dBZ,&col[c].R,&col[c].G,&col[c].B);
      col[c].N=(uchar)(2.0*dBZ+64.0);    
      printf("VÄRIT %d %d %d %d\n",col[c].N,col[c].R,col[c].G,col[c].B);
      c++;
   }
   fclose(CLASSFILE);
   printf("VÄRIT %d LUETTU\n",c);
   col[c].N=254;
   col[c].R=col[c-1].R;
   col[c].G=col[c-1].G;
   col[c].B=col[c-1].B;
   colind=c;

   legend=gdImageCreate(300,40);
      sprintf(mapfile,"%s/Suomi.ppm",argpath);
      maparr=malloc(arrsize*3);
      MAPFILE=fopen(mapfile,"r");
      fgets(sline,199,MAPFILE);
      fgets(sline,199,MAPFILE);
      fgets(sline,199,MAPFILE);
      fread(maparr,arrsize*3,1,MAPFILE);
      fontcolor=gdImageColorAllocate(legend,255,255,255);
   fclose(MAPFILE);
   printf("KARTTA %s LUETTU\n",mapfile);

   outarr=calloc(arrsize*3,1);
   sprintf(outfile,"%s/%s",picpath,argv[2]);
   OUTFILE=fopen(outfile,"w");
   fprintf(OUTFILE,"P6\n%ld %ld\n255\n",Xdim,Ydim);

   N=3*750;
   bgcolor=gdImageColorAllocate(legend,maparr[N],maparr[N+1],maparr[N+2]); 


        sprintf(legtext,"Ensemble mean dBZ ");
        sprintf(legtime,"%.2s.%.2s.%.4s %.2s:%.2s -> %.2s:%.2s UTC",
                infile+8,infile+6,infile+2,infile+10,infile+12,
                infile+23,infile+25);
        inarr=malloc(arrsize);
        INFILE=fopen(argv[1],"r");
        fgets(sline,199,INFILE);
        if(sline[0]!='P') rewind(INFILE);
        fread(inarr,arrsize,1,INFILE);
        fclose(INFILE); 
  
   printf("%s\n%s\n",legtext,legtime);


       for(n=0;n<arrsize;n++)
       {
          N=3*n;
          mr=maparr[N];
          c=inarr[n];
          if((c<col[0].N) || (c==255))
          {
            outarr[N]=mr;
            outarr[N+1]=mr;
            outarr[N+2]=mr;
          }        
          else
          {
            if(!mr)
            {
              r=255;
              g=255;
	      b=255;
            }
            else
	    {
              for(i=0;i<colind;i++)
              if((c>=col[i].N) && (c<col[i+1].N)) 
              {
                r=col[i].R;
                g=col[i].G;
	        b=col[i].B;
              }
            }
            outarr[N]=r;
            outarr[N+1]=g;
            outarr[N+2]=b;
          } 
       }   

   printf("DATA %c LUETTU\n",maptype);

   gdImageFilledRectangle(legend,0,0,299,39,bgcolor);
   gdImageString(legend, gdFontLarge,1,3,legtext,fontcolor);
   gdImageString(legend, gdFontLarge,1,20,legtime,fontcolor);
   insert_legend(outarr,legend,-1,-1);
   gdImageDestroy(legend);

   fwrite(outarr,arrsize*3,1,OUTFILE);
   fclose(OUTFILE);

   free(maparr);
   free(inarr);
   free(outarr);

   return(0);
}


void insert_legend
    (uchar *maparr, gdImagePtr legend, long int xpos, long int ypos)
{
   ulong i,lsize,lx,ly,x,y,lN,pN;
   uchar *legarr;


   lx=legend->sx;
   ly=legend->sy;

   if(xpos<0) xpos=Xdim+xpos-lx-1;
   if(ypos<0) ypos=Ydim+ypos-ly-1;

   lsize=3*lx*ly;
   legarr=malloc(lsize);

   GD_to_ppm(legarr,legend);
   lN=0;
   for(y=ypos;y<(ypos+ly);y++)
   {
     for(x=xpos;x<(xpos+lx);x++)
     {
        pN=3*(y*Xdim+x);
        for(i=0;i<3;i++) maparr[pN+i]=legarr[lN+i];
        lN+=3;
     }
   }
   free(legarr);
   return;
}        

void GD_to_ppm(unsigned char *ppmarr, gdImagePtr im)
{

   long int N;
   int x,y,c;

   N=0;
   for(y=0;y<im->sy;y++) for(x=0;x<im->sx;x++)
   {
        c=gdImageGetPixel(im,x,y);
        ppmarr[N]=im->red[c];
        ppmarr[N+1]=im->green[c];
        ppmarr[N+2]=im->blue[c];
        N+=3;
   }

   return;
}


struct tm utc_to_local(char *utc, long int timediff)
{
   struct tm Sdd,Ldd;
   time_t secs,l_time,tdiff;

   l_time=time(&l_time);
   Ldd=*localtime(&l_time);
   if(Ldd.tm_isdst==1) tdiff=10800; else tdiff=7200;

   sscanf(utc,"%4d%2d%2d%2d%2d",&Sdd.tm_year,&Sdd.tm_mon,
                                 &Sdd.tm_mday,&Sdd.tm_hour,
                                 &Sdd.tm_min);
   Sdd.tm_year-=1900;
   Sdd.tm_mon--;
   Sdd.tm_sec=0;
   Sdd.tm_isdst=-1;
   secs=mktime(&Sdd);
   secs=secs+tdiff+timediff;
   Ldd=*localtime(&secs);     
   return(Ldd);
}







