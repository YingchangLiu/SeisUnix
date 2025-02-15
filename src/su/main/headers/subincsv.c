/* Copyright (c) Colorado School of Mines, 2021.*/
/* All rights reserved.                       */

/* SUBINCSV: $Revision: 1.1 $ ; $Date: 2021/10/03 23:44:55 $         */

#include <stdio.h>
#include <string.h>
#include "math.h"

#include "su.h"
#include "segy.h"
#include "readkfile.h"  
#include "gridxy.h"

segy tr;

int GetCase(char* cbuf) ;
double fromhead(segy tr, int k) ;

/*********************** self documentation **********************/
char *sdoc[] = {
"                          ",
" SUBINCSV - 3D Grid Resolve and output and assign traces to cdps (cells). ",
"                                                                          ",
" subincsv  rfile=in.csv wfile=out.csv <in.su >out.su                      ",
"                                                                          ",
" Parameter overview:                                                      ",
"                                                                          ",
"       rfile= if specified, read a K-file containing parameter values.    ", 
"       wfile= if specified, write a K-file with parameter values.         ", 
"                                                                          ",
"       in.su and out.su do not have to be specified (if you just want     ",
"       to specify command line parameters and get the output K-file).     ",
"                                                                          ",
"       Parameters specified on the command line override parameters from  ",
"       the rfile.                                                         ",
"                                                                          ",
"       bintype=        See below for which other parameters are required  ",
"                       for the option numbers here.                       ",
"               30      Compute trace midpoint coordinates (sx+gx)/2 and   ",
"                       (gx+gy)/2 and update cdp,igi,igc keys (grid cell   ",
"                       number and inline and crossline index numbers).    ",
"                       Option 30 is usually used for prestack traces.     ",
"              -30      Use input cdp (cell) number and update keys        ",
"                       igi,igc,sx,sy,gx,gy. Where igi,igc are grid index  ",
"                       numbers of the cdp (cell) number and sx,sy is the  ",
"                       cell centre in raw coordinates and gx,gy is cell   ",
"                       centre in grid coordinates (which are shifted and  ",
"                       aligned with grid definition, but not scaled by    ",
"                       cell widths). Note that sx,sy are only approximate ",
"                       cell centres since scalco rounds after sin,cosine  ",
"                       computations, but gx,gy are usually much more      ",
"                       precise since they are multiples of cell widths.   ",
"                       Option -30 is usually used for poststack traces.   ",
"              -31      Use input cdp (cell) number and update igi,igc.    ",
"              -32      Use input igi,igc and update cdp number as well as ",
"                       sx,sy,gx,gy as described in option -30.            ",
"               20      Use point numbers to compute a 2d cdp number.      ",
"                       Option 20 is usually used for prestack traces.     ",
"                                                                          ",
"                                                                          ",
"       offset=         By default, bintype=30 and 20 recompute offset,    ",
"                       but other bintypes leave it as-is.                 ",
"             =1        Recompute offset key.                              ",
"             =0        Do not recompute offset key.                       ",
"                                                                          ",
"       check=0         Do not print checking details.                     ",
"             1         For grid bintypes, after grid defintion is set,    ",
"                       run some grid functions on the 4 corner points     ",
"                       and print the results. The intention here is to    ",
"                       exercise many functions in case of issues created  ",
"                       by coding or compiler or optimizer errors/changes. ",
"                       But the output print may also be useful for users. ",
"                       For instance, you will see slight differences in   ",
"                       the coordinates of those 4 corners when produced   ",
"                       by different functions, and when run on different  ",
"                       hardware or with different compilers/optimizers.   ",
"                                                                          ",
" Grid parameters (required, either on command line or in rfile).          ",
"                                                                          ",
"    grid_xa=  X coordinate of corner A.                                   ",
"    grid_ya=  Y coordinate of corner A.                                   ",
"    grid_xb=  X coordinate for corner B.                                  ",
"    grid_yb=  Y coordinate for corner B.                                  ",
"    grid_xc=  X coordinate for corner C.                                  ",
"    grid_yc=  Y coordinate for corner C.                                  ",
"    grid_wb=  width of cells in A-->B direction.                          ",
"    grid_wc=  width of cells in A-->C direction.                          ",
"                                                                          ",
" Note that corner A coordinates are used exactly, but corner B is reset   ",
" to an exact multiple distance of the cell width in A-->B direction.      ",
" And corner C is reset onto a line at right angle to A-->B direction      ",
" through A and also to an exact multiple distance of A-->C cell width.    ",
"                                                                          ",
" -----------------------------------------------------------------------  ",
"                                                                          ",
" Cdps from Points parameters (either on command line or in rfile).        ",
"                                                                          ",
"    point_crz= cdp number of receiver zero                                ",
"    point_cru= cdps per one receiver point unit                           ",
"    point_csz= cdp number of source zero                                  ",
"    point_csu= cdps per one source point unit                             ",
"                                                                          ",
" Cdps from Points parameters (only on command line).                      ",
"                                                                          ",
"    rpkey=     key containing receiver point numbers (default is gaps)    ",
"    spkey=     key containing source point numbers (default is grnlof).   ",
"               Note these defaults are same as sutoolcsv and sugeomcsv.   ",
"                                                                          ",
" The computed cdp value is simply:                                        ",
"   cdp = (rpkey*point_cru + point_crz + spkey*point_csu + point_csz) / 2. ",
" with value rounded to nearest integer.                                   ",
"                                                                          ",
" Note: The survey does not need an actual receiver or source point zero.  ",
"                                                                          ",
" Example: Assuming a 2D situation where you want 2 cdps per receiver.     ",
"          If receiver points are numbered from 100 to 1000 by 1, then     ",
"          point_crz=0 and point_cru=2 means receiver point 100 will       ",
"          have a cdp number of 200 and receiver 101 will have cdp 202.    ",
"          But if receivers are numbered 100 to 10000 by 10, then you can  ",
"          specify point_cru=0.2 and receiver point 100 will have cdp 20   ",
"          and receiver point 110 will have cdp 22. If you want, you can   ",
"          set point_crz=180 to get the same cdp numbers as the first      ",
"          situation. Both point_cru and point_csu can be negative. This   ",
"          means your cdps will number backwards relative to the point     ",
"          numbering, which is sometimes desirable/required. But you       ",
"          should set point_crz and point_csz to avoid negative cdps.      ",
"          Exactly the same computations are used with point_csz and       ",
"          point_csu to produce the cdp number of the sources (shots).     ",
"          Often for 2d surveys, the same point numbering values have been ",
"          used for rpkey and spkey, so the parameters should be the same. ",
"          An exception to this is when the sources are skidded to halfway ",
"          between receivers. Even though the source is given the same     ",
"          point number, you should adjust point_csz slightly to get the   ",
"          most accurate cdp number at sources. Sometimes, of course, the  ",
"          source point numbering will be completely unrelated to the      ",
"          receiver point numbering, so you will have to specify different ",
"          parameter values to get corresponding cdp numbering.            ",
"          And there are situations where you want to produce 4 cdps per   ",
"          receiver and source (or 3 or 1 or whatever). That just requires ",
"          setting point_cru and point_csu properly. In any case, once     ",
"          computed the trace receiver cdp and the trace source cdp are    ",
"          summed and divided by 2 for the output trace cdp key value.     ",
"                                                                          ",
" Advice:  OK, why not just use the grid options with a very wide cell     ",
"          width in the A-->C direction and put corner C on corner A?      ",
"          That creates one cell in the A-->C direction and will function. ",
"          If your input coordinates are just simple increments along a    ",
"          straight line, then the wide-cell will make cdps having a nice  ",
"          fold. But with actual surveyed coordinates, the cdp fold is     ",
"          likely to vary a lot (with some cdps having roughtly twice the  ",
"          average fold and some cdps having low fold (and not right next  ",
"          to each other either). This occurs because surveyed coordinates ",
"          tend to produce trace midpoints in clumps, and those clumps are ",
"          not exactly the same interval apart. Slowly the cell boundaries ",
"          in the A-->B direction get misaligned with those clumps. So,    ",
"          at some location, two clumps get into one cell, and at other    ",
"          locations, no clump gets into a cell. (Even when the cdp fold   ",
"          is well-behaved, the offset distribution for wide-cell cdps     ",
"          tends to vary a lot).                                           ",
"          On the other hand, points tend to be numbered exactly as the    ",
"          survey was designed. So using them means you get cdps that have ",
"          the designed fold and offset distribution. But it also means    ",
"          that cmps may have traces which spatially overlap other cmps.   ",
"          In other words, wide-cell grids make spatially exact boundaries ",
"          at the expense of nice fold and offset distribution, whereas    ",
"          points can make nice fold and offset distribution at the        ",
"          expense of exact spatial boundaries.                            ",
"                                                                          ",
" ***********************************************************              ",
"   To output this documentation:  subincsv 2> bindoc.txt                  ",
" ***********************************************************              ",
"                                                                          ",
" Seismic Unix has a 240 byte header which already has defined key names.  ",
" In other seismic processing systems the ability to expand trace headers  ",
" and insert intricate grid transform values is both a blessing and a curse",
" (trust me on that). For SU, intricate grid-related values have no keys   ",
" to be stored. So only 3 grid values are updated (to keys cdp, igi, igc). ",
" Where igi is set to the cell index in the direction from corner A to     ",
" corner B (a mnemonic for igi is index-grid-inline). And where igc is set ",
" to cell index in the direction from corner A to corner C (a mnemonic for ",
" igc is index-grid-crossline).                                            ",
"                                                                          ",
" The input grid definition command line parameters are processed and      ",
" written to an external file. That file follows conventions established   ",
" by SUTOOLCSV and SUGEOMCSV. I call this the K-file (K for Konstants).    ",
" Reading this K-file should allow other SU programs to perform intricate  ",
" grid transforms on-the-fly using sx,sy,gx,gy coordinates as well as      ",
" backwards transforms from cdp,igi,igc to cell centre XYs. The grid in    ",
" the K-file will also allow transforms of XYs values in S and R tables in ",
" spreadsheets (eventually). And, once output, the K-file can be re-input  ",
" to this program instead of using command line parameters.                ",
"                                                                          ",
" The grid will be an exact rectangle with 4 corner points A,B,C, and D.   ",
" But you can only specify XYs for corners A,B,C (D is computed herein).   ",
" Corner A coordinates are used exactly as input. Then the direction from  ", 
" corner A to input corner B is determined exactly. After that, corner B   ", 
" coordinates are adjusted to an exact multiple of the cell width you      ", 
" specify for the A-->B direction. Then your input coordinates of corner C ", 
" are used to compute the distance from corner A to corner C. The right    ", 
" angle to A-->B gives direction for output corner C (along line thru A).  ", 
" Corner C is then adjusted to an exact multiple of the cell width you     ", 
" specify for the A-->C direction. Note this means input corner C only     ", 
" determines how wide the rectangle is, and which side of A-->B the        ",
" processed corner C is on. Corner D is computed from the other corners.   ",
"                                                                          ",
" The first cell is centred on corner A and has igi=1 and igc=1.           ",
" Cell centres then increment by their corresponding widths in the A-->B   ",
" and A-->C directions. The igi and igc values increment by 1. cdp numbers ",
" start at 1 in the first cell and increment by 1 in the A-->B direction   ",
" until reaching corner B, then moves 1 cell in the A-->C direction        ",
" (near corner A) and continues to increment by 1 in the A-->B direction.  ",
"                                                                          ",
" Cells only contain one-half of their boundaries. This ensures that a     ",
" trace midpoint that is exactly between 2 or 4 cell centres is assigned   ",
" to a specific cell. Note: This is why you cannot use proximity to cell   ",
" centres to assign traces to cells. You need to actually compute the cdp  ",
" and igi,igc numbers the way that it is done herein.                      ",
"                                                                          ",
" For bintypes that use the grid, grid_xa,grid_ya,grid_xb,grid_yb,         ",
" grid_xc,grid_yc,grid_wb,grid_wc must be specified either on the          ", 
" command line or in the input K-file. These values are the XYs of         ", 
" corners A,B,C as well as the cell widths. The output K-file contains     ",
" the following processed grid definition values:                          ",
"                                                                          ",
"              Name      Definition                                        ",
"              ----      ----------                                        ",
"                                                                          ",
"             bintype = bintype number                                     ",
"             grid_lf = which side of A-->B is C on? 1=left, -1=right      ",
"             grid_xa = raw, real world X coordinate of corner A           ",
"             grid_ya = raw, real world Y coordinate of corner A           ",
"             grid_xb = raw, real world X coordinate of corner B           ",
"             grid_yb = raw, real world Y coordinate of corner B           ",
"             grid_xc = raw, real world X coordinate of corner C           ",
"             grid_yc = raw, real world Y coordinate of corner C           ",
"             grid_xd = raw, real world X coordinate of corner D           ",
"             grid_yd = raw, real world Y coordinate of corner D           ",
"             grid_wb = width of cells in A-->B direction                  ",
"             grid_wc = width of cells in A-->C direction                  ",
"             grid_nb = number of cells in A-->B direction                 ",
"             grid_nc = number of cells in A-->C direction                 ",
"             grid_fp = first cdp (cell) number                            ",
"             grid_lp = last  cdp (cell) number                            ",
"             grid_sb = sine   of A-->B to X-axis.                         ",
"             grid_cb = cosine of A-->B to X-axis.                         ",
"                                                                          ",
"  Note that corners A,B,C,D are at the centres of cells. Thus coordinates ",
"  can be half a cell width outside A,B,C,D and still be inside the grid.  ",
"                                                                          ",
" Warning and advice:                                                      ",
"  Cell boundaries and other grid computations use double precision values ",
"  and are therefore extremely precise. This very precision causes issues. ",
"  When a trace midpoint is very near a cell boundary, it only takes a     ",
"  slight difference in hardware/compiler/optimizer computations for the   ",
"  boundaries to move a bit, and therefore assign some traces to different ",
"  cells. You should expect that. Similarly, trying to reverse or invert a ",
"  grid by exchanging corners A and B and so on, is also not likely to     ",
"  result in exactly the same distribution of traces in the cells.         ",
"                                                                          ",
"  -------------------------------------------------------------------     ",
"                                                                          ",
"  For bintype=20, point_crz,point_cru,point_csz,point_csu must be         ",
"  specified either on the command line or in the input K-file.            ",
"  The output K-file contains the same values:                             ",
"                                                                          ",
"              Name      Definition                                        ",
"              ----      ----------                                        ",
"                                                                          ",
"             bintype   = bintype number                                   ",
"             point_crz = cdp number of receiver zero                      ",
"             point_cru = cdps per one receiver point unit                 ",
"             point_csz = cdp number of source zero                        ",   
"             point_csu = cdps per one source point unit                   ",
"                                                                          ",
" Note that K-files can contain these values in any order, and             ",
" K-files can contain other values that this program does not use.         ",
"                                                                          ",
" ----------------------------------------------------------------------   ", 
"                                                                          ",
NULL};

/* Credits:                                                       */
/* Andre Latour                                                   */ 
/*                                                                */
/* Started from sutoolcsv and sugeomcsv.                          */
/* This program has deliberatly been divided into functions to    */
/* make it easier to use the grid in other programs. I would      */
/* appreciate it if future coders did not take shortcuts. Do not  */
/* copy the code from inside the functions, use the functions.    */
/* If you need some grid manipulation that is not in an extant    */
/* function, then write a new function.                           */
/*                                                                */
/* Usual trace keys: sx,sy,gx,gy,scalco,cdp,igi,igc,offset.       */
/*                                                                */
/**************** end self doc ************************************/

int main(int argc, char **argv) {

  cwp_String Rname=NULL;  /* text file name for values            */
  FILE *fpR=NULL;         /* file pointer for Rname input file    */
  cwp_String Wname=NULL;  /* text file name for output values     */
  FILE *fpW=NULL;         /* file pointer for Wname output file   */

  cwp_String spkey=NULL;   /* key for input source point values   */
  int spcase = 0;

  cwp_String rpkey=NULL;   /* key for input receiver point values */
  int rpcase = 0;

/* Most of following are much bigger than will be used.            */    
/* But difficult to dynamically allocate since they often will be  */    
/* read-in from C_SU_ records in the input text file.              */    

  cwp_String names[999];   
  cwp_String forms[999];   
  double dfield[999];

  cwp_String gnams[999];   
  double gvals[999];

  int nproct = 0; 
  int i=0;
  int j=0;
  int n=0;
  int m=0;

/* Initialize */
  initargs(argc, argv);
  requestdoc(1);

  getparstring("rfile", &Rname);
  getparstring("wfile", &Wname);

  if(Rname != NULL && Wname != NULL && strcmp(Rname,Wname) == 0) 
    err("**** Error: wfile= output K-file must be different than rfile= input K-file.");

  int intraces = 1;
  if(isatty(STDIN_FILENO)==1) { /* do not have input trace file */
    intraces = 0;
    if (Wname == NULL) err("**** Error: wfile= output K-file name must be specified when no input traces.");
    if(isatty(STDOUT_FILENO)!=1) { /* have output trace file */
      err("**** Error: Cannot specify output trace file with no input trace file.");
    }
  }
  else {
    if(isatty(STDOUT_FILENO)==1) { /* do not have output trace file */
      err("**** Error: Must have output trace file when input trace file is specified.");
    }
  }

  int bintype;
  if (!getparint("bintype", &bintype)) bintype = -1;
  
  int ioffset;
  if (!getparint("offset", &ioffset)) ioffset = -1;
  
  int icheck;
  if (!getparint("check", &icheck)) icheck = 0;

/* Cycle over rfile records to get some C_SU_ parameters? */ 

  int numcases = 0;
  int errwarn;

  if (Rname != NULL) {
    fpR = fopen(Rname, "r");
    if (fpR == NULL) err("rfile error: input K-file did not open correctly.");

    readkfile(fpR,names,forms,dfield,&numcases,&errwarn);
     
    if(errwarn==1) err("K-file read error: more than one C_SU_NAMES parameter record.");
    else if(errwarn==2) err("K-file read error: no C_SU_NAMES parameter record.");
    else if(errwarn==3) err("K-file read error: more than one C_SU_FORMS parameter record.");
    else if(errwarn==4) err("K-file read error: no C_SU_FORMS parameter record.");
    else if(errwarn==5) err("K-file read error: more than one C_SU_SETID record.");
    else if(errwarn==6) err("K-file read error: no C_SU_SETID record.");
    else if(errwarn==7) err("K-file read error: different number of values on C_SU_NAMES and C_SU_FORMS.");
    else if(errwarn==8) err("K-file read error: unable to allocate memory.");
    else if(errwarn==9) err("K-file read error: name exists at least twice in C_SU_NAMES list.");
    else if(errwarn==10) err("K-file read error: at least 1 field-unreadable as a number.");
    else if(errwarn==11) err("K-file read error: at least 1 field containing 2 numbers.");
    else if(errwarn==12) err("K-file read error: not-enough-commas to get all values for C_SU_NAMES list.");
    else if(errwarn>0) err("K-file read error: returned with some unrecognized error code.");
    else if(errwarn==-1) warn("K-file read warning: at least 1 all-blank field, assumed zero for all.");

    for (n=0; n<numcases; n++) {
      for (m=0; m<strlen(names[n]); m++) {
        names[n][m] = tolower(names[n][m]);
      }
    }    

  } /* end of  else for if (Rname == NULL) { */

/* ----------------------------------------------------------- */

  int numcasesout = numcases;

  int numgnams = 1;
  gvals[0] = -1.1e308;
  gnams[0] = ealloc1(7,1); 
  strcpy(gnams[0],"bintype");

  if(bintype==-1) {
    gvals[0] = -1.;
    for(j=0; j<numcases; j++) { 
      if(strcmp(names[j],gnams[0]) == 0) gvals[0] = dfield[j]; 
    }
    if(gvals[0] > 0.) bintype = (int) (gvals[0] + 0.1);
    else bintype = (int) (gvals[0] - 0.1);
  }
  else {
    gvals[0] = bintype;
  }

  if(bintype!=30 && bintype!=-30 && bintype!=-31 && bintype!=-32 && bintype!=20) {
    err("**** Error: bintype number %d is not recognized.",bintype);
  }

  if(ioffset==-1) {
    if(bintype==30 || bintype==20) ioffset = 1;
    else ioffset = 0;
  }

/* Process and set the grid definition values?                                   */

  if(bintype==30 || bintype==-30 || bintype==-31 || bintype==-32) {

    numgnams = 18;

    for(i=1; i<numgnams; i++) {
      gvals[i] = -1.1e308;
      gnams[i] = ealloc1(7,1); 
    }

    strcpy(gnams[1],"grid_lf");
    strcpy(gnams[2],"grid_xa");
    strcpy(gnams[3],"grid_ya");
    strcpy(gnams[4],"grid_xb");
    strcpy(gnams[5],"grid_yb");
    strcpy(gnams[6],"grid_xc");
    strcpy(gnams[7],"grid_yc");
    strcpy(gnams[8],"grid_xd");
    strcpy(gnams[9],"grid_yd");
    strcpy(gnams[10],"grid_wb");
    strcpy(gnams[11],"grid_wc");
    strcpy(gnams[12],"grid_nb");
    strcpy(gnams[13],"grid_nc");
    strcpy(gnams[14],"grid_fp");
    strcpy(gnams[15],"grid_lp");
    strcpy(gnams[16],"grid_sb");
    strcpy(gnams[17],"grid_cb");

    for(i=2; i<12; i++) {    /* read-in corners A,B,C and cell widths */ 
      if(i==8 || i==9) continue; /* do not read-in corner D  */
      if(!getpardouble(gnams[i],gvals+i)) { 
        for(j=0; j<numcases; j++) { 
          if(strcmp(names[j],gnams[i]) == 0) gvals[i] = dfield[j]; 
        }
        if(gvals[i] < -1.e308) {
          gvals[i] = i+100;
          err("Error: bintype=%d and parameter %s not found.",bintype,gnams[i]); 
        }
      }
    } /* end of  for(int i=2; i<12; i++) { */

/* Process and set other grid values. */

    int errwarn;
    gridset(gvals,&errwarn); 

    if(errwarn==1) err ("gridset error: grid_wb cell width must be positive.");
    else if(errwarn==2) err ("gridset error: grid_wc cell width must be positive.");
    else if(errwarn==3) err ("gridset error: corner B is within grid_wb cell width of corner A.");
    else if(errwarn>0) err ("gridset error: returned with some unrecognized error code.");
    else if(errwarn==-1) warn ("gridset warning: corner C is near A and is reset to A.");

    gridcheck(gvals,icheck,&errwarn); 
    if(errwarn>0) err ("gridcheck error: returned with some unrecognized error code.");

  } /* end of  if(bintype==30 || ..... */
  else if(bintype==20) {

    if (!getparstring("rpkey", &rpkey)) rpkey = "gaps";
    rpcase = GetCase(rpkey);
    if(rpcase<1) err("Error: rpkey= %s is not recognized.",rpkey);
    if (!getparstring("spkey", &spkey)) spkey = "grnlof";
    spcase = GetCase(spkey);
    if(spcase<1) err("Error: spkey= %s is not recognized.",spkey);

    numgnams = 5;

    for(i=1; i<numgnams; i++) {
      gvals[i] = -1.1e308;
      gnams[i] = ealloc1(9,1); 
    }

    strcpy(gnams[1],"point_crz"); /* cdp number of receiver zero      */
    strcpy(gnams[2],"point_cru"); /* cdps per one receiver point unit */
    strcpy(gnams[3],"point_csz"); /* cdp number of source zero        */
    strcpy(gnams[4],"point_csu"); /* cdps per one source point unit   */

    for(i=1; i<5; i++) {     
      if(!getpardouble(gnams[i],gvals+i)) { 
        for(j=0; j<numcases; j++) { 
          if(strcmp(names[j],gnams[i]) == 0) gvals[i] = dfield[j];  
        }
        if(gvals[i] < -1.e308) {
          gvals[i] = i+100;
          err("**** Error: bintype=%d and parameter %s not found.",bintype,gnams[i]); 
        }
      }
    } /* end of  for(i=1; i<5; i++) { */

  } /* end of  if(bintype==20) { */

/* -----------------------------------------------------------    */
/*  If outputting a text file, open it.... */

  if (Wname != NULL) {

    fpW = fopen(Wname, "w");
    if (fpW == NULL) err("wfile error: output K-file did not open correctly.");

/* Update/add whatever names/values are used by whatever options.             */

    for(i=0; i<numgnams; i++) { 

      int ifound = 0;
      for(j=0; j<numcases; j++) { /* reset it, if already in input K-file */ 
        if(strcmp(names[j],gnams[i]) == 0) {
          dfield[j] = gvals[i];  
          ifound = 1;
          break;
        }
      }
      if(ifound == 0) {               /* or add it */         
        dfield[numcasesout] = gvals[i];
        names[numcasesout] = ealloc1(7,1);
        strcpy(names[numcasesout],gnams[i]);
        forms[numcasesout] = ealloc1(5,1);
        strcpy(forms[numcasesout],"%.20g");
        numcasesout++;
      }

    } /* end of  for(i=0; i<numgnams; i++) { */

    writekfile(fpW,names,forms,dfield,numcasesout,&errwarn);
    if(errwarn>0) err("K-file write error: returned with an unrecognized error code.");

  } /* end of  if (Wname != NULL) { */

/* -----------------------------------------------------------    */

  if(intraces==0) return(0);

/* -----------------------------------------------------------    */

  if (!gettr(&tr))  err("Error: cannot get first trace");

  double dx;
  double dy;
  double tx;
  double ty;
  int icdp;
  int igi;
  int igc;

/* loop over traces   */ 

  do {

    if(ioffset==1) {
      dx = tr.sx;
      dy = tr.sy;
      tx = tr.gx;
      ty = tr.gy;

      if(tr.scalco > 1) { 
        dx *= tr.scalco;
        dy *= tr.scalco;
        tx *= tr.scalco;
        ty *= tr.scalco;
      }
      else if(tr.scalco < 0) { /* note that 1 and 0 retain values as-is */
        dx /= -tr.scalco;
        dy /= -tr.scalco;
        tx /= -tr.scalco;
        ty /= -tr.scalco;
      }
      tr.offset = lrint(sqrt((dx-tx)*(dx-tx) + (dy-ty)*(dy-ty)));
    }

/* apply bintypes   */ 

    if(bintype==30) {

      dx = 0.5 * (double)(tr.sx + tr.gx);
      dy = 0.5 * (double)(tr.sy + tr.gy);

      if(tr.scalco > 1) { 
        dx *= tr.scalco;
        dy *= tr.scalco;
      }
      else if(tr.scalco < 0) { 
        dx /= -tr.scalco;
        dy /= -tr.scalco;
      }

/* Compute cdp,igi,igc from coordinates.                                */

      gridrawxycdpic(gvals,dx,dy,&icdp,&igi,&igc);
      if(icdp<-2147483644) 
        err("Error: input midpoint XYs not in grid (cannot compute cdp number). Trace= %d",nproct+1);

      tr.cdp = icdp;
      tr.igi = igi; 
      tr.igc = igc;


/* The following commented-out code is intended to exercise and check some of the grid     */
/* routines. If the sine,cosine,leftright is coded correctly then trace raw XYs to         */
/* grid XYs and then vice-versa should produce nearly the same raw XYs. And, igi,igc       */
/* should produce cell centre raw XYs that are never more than half a cell width away from */
/* trace raw XYs. (I just check within 99 in the code below since the concern is not small */
/* differences, it is large differences induced by wrong sine,cosine,leftright logic).     */
/*      if(icheck>0) {                                                                     */
/*      gridrawxygridxy(gvals,dx,dy,&tx,&ty);                                              */ 
/*      double rx;                                                                         */
/*      double ry;                                                                         */
/*      gridgridxyrawxy(gvals,tx,ty,&rx,&ry);                                              */ 
/*      double cx;                                                                         */
/*      double cy;                                                                         */
/*      gridicrawxy(gvals,igi,igc,&cx,&cy);                                                */ 
/*      if(fabs(dx-rx)>0.0001 || fabs(dy-ry)>0.0001 || fabs(dx-cx)>99. || fabs(dy-cy)>99.  */ 
/*           || (nproct>icheck-5 && nproct<icheck+5)) {                                    */
/*       warn("check %f %f %f %f %f %f %f %f Trace= %d",dx,dy,tx,ty,rx,ry,cx,cy,nproct+1); */
/*      }                                                                                  */ 
/*    }                                                                                    */


    } /* end of  if(bintype==30) { */
    else if(bintype==-30 || bintype==-31) {
      icdp = tr.cdp;
      gridcdpic(gvals,icdp,&igi,&igc);
      if(igi<-2147483644) 
        err("Error: input cdp number not in grid (cannot compute igi,igc). Trace= %d",nproct+1);
      tr.igi = igi;
      tr.igc = igc;
    } 
    else if(bintype==-32) {
      igi = tr.igi;
      igc = tr.igc;
      gridiccdp(gvals,igi,igc,&icdp); 
      if(icdp<-2147483644) 
        err("Error: input igi,igc numbers not in grid (cannot compute cdp). Trace= %d",nproct+1);
      tr.cdp = icdp;
    } 
    else if(bintype==20) {
     tx = fromhead(tr, rpcase) * gvals[2] + gvals[1];
     ty = fromhead(tr, spcase) * gvals[4] + gvals[3];
     tr.cdp = lrint((tx+ty)/2.);
    }

/* Finish off these grid options */

    if(bintype==-30 || bintype==-32) {

      gridicrawxy(gvals,igi,igc,&dx,&dy);
      gridicgridxy(gvals,igi,igc,&tx,&ty);

      if(tr.scalco > 1) { 
        dx /= tr.scalco;
        dy /= tr.scalco;
        tx /= tr.scalco;
        ty /= tr.scalco;
      }
      else if(tr.scalco < 0) { 
        dx *= -tr.scalco;
        dy *= -tr.scalco;
        tx *= -tr.scalco;
        ty *= -tr.scalco;
      }
      tr.sx = dx;
      tr.sy = dy;
      tr.gx = tx;
      tr.gy = ty;
    } 

    puttr(&tr);
    nproct++;

  } while (gettr(&tr));

  warn("Number of traces %d ",nproct);

  return 0;

}

/* -------------------------------------------- */

int GetCase(char* cbuf) {
   
       int ncase = -1;
   
       if(strncmp(cbuf,"null",4) == 0) ncase = 0;  /* any name starting with null */
       else if(strcmp(cbuf,"tracl") == 0) ncase = 1;
       else if(strcmp(cbuf,"tracr") == 0) ncase = 2;
       else if(strcmp(cbuf,"fldr" ) == 0) ncase = 3;
       else if(strcmp(cbuf,"tracf") == 0) ncase = 4;
       else if(strcmp(cbuf,"ep"   ) == 0) ncase = 5;
       else if(strcmp(cbuf,"cdp") == 0) ncase = 6;
       else if(strcmp(cbuf,"cdpt") == 0) ncase = 7;
       else if(strcmp(cbuf,"trid") == 0) ncase = 8;
       else if(strcmp(cbuf,"nvs") == 0) ncase = 9;
       else if(strcmp(cbuf,"nhs") == 0) ncase = 10;
       else if(strcmp(cbuf,"duse") == 0) ncase = 11;
       else if(strcmp(cbuf,"offset") == 0) ncase = 12;
       else if(strcmp(cbuf,"gelev") == 0) ncase = 13;
       else if(strcmp(cbuf,"selev") == 0) ncase = 14;
       else if(strcmp(cbuf,"sdepth") == 0) ncase = 15;
       else if(strcmp(cbuf,"gdel") == 0) ncase = 16;
       else if(strcmp(cbuf,"sdel") == 0) ncase = 17;
       else if(strcmp(cbuf,"swdep") == 0) ncase = 18;
       else if(strcmp(cbuf,"gwdep") == 0) ncase = 19;
       else if(strcmp(cbuf,"scalel") == 0) ncase = 20;
       else if(strcmp(cbuf,"scalco") == 0) ncase = 21;
       else if(strcmp(cbuf,"sx") == 0) ncase = 22;
       else if(strcmp(cbuf,"sy") == 0) ncase = 23;
       else if(strcmp(cbuf,"gx") == 0) ncase = 24;
       else if(strcmp(cbuf,"gy") == 0) ncase = 25;
       else if(strcmp(cbuf,"counit") == 0) ncase = 26;
       else if(strcmp(cbuf,"wevel") == 0) ncase = 27;
       else if(strcmp(cbuf,"swevel") == 0) ncase = 28;
       else if(strcmp(cbuf,"sut") == 0) ncase = 29;
       else if(strcmp(cbuf,"gut") == 0) ncase = 30;
       else if(strcmp(cbuf,"sstat") == 0) ncase = 31;
       else if(strcmp(cbuf,"gstat") == 0) ncase = 32;
       else if(strcmp(cbuf,"tstat") == 0) ncase = 33;
       else if(strcmp(cbuf,"laga") == 0) ncase = 34;
       else if(strcmp(cbuf,"lagb") == 0) ncase = 35;
       else if(strcmp(cbuf,"delrt") == 0) ncase = 36;
       else if(strcmp(cbuf,"muts") == 0) ncase = 37;
       else if(strcmp(cbuf,"mute") == 0) ncase = 38;
       else if(strcmp(cbuf,"ns") == 0) ncase = 39;
       else if(strcmp(cbuf,"dt") == 0) ncase = 40;
       else if(strcmp(cbuf,"gain") == 0) ncase = 41;
       else if(strcmp(cbuf,"igc") == 0) ncase = 42;
       else if(strcmp(cbuf,"igi") == 0) ncase = 43;
       else if(strcmp(cbuf,"corr") == 0) ncase = 44;
       else if(strcmp(cbuf,"sfs") == 0) ncase = 45;
       else if(strcmp(cbuf,"sfe") == 0) ncase = 46;
       else if(strcmp(cbuf,"slen") == 0) ncase = 47;
       else if(strcmp(cbuf,"styp") == 0) ncase = 48;
       else if(strcmp(cbuf,"stas") == 0) ncase = 49;
       else if(strcmp(cbuf,"stae") == 0) ncase = 50;
       else if(strcmp(cbuf,"tatyp") == 0) ncase = 51;
       else if(strcmp(cbuf,"afilf") == 0) ncase = 52;
       else if(strcmp(cbuf,"afils") == 0) ncase = 53;
       else if(strcmp(cbuf,"nofilf") == 0) ncase =54;
       else if(strcmp(cbuf,"nofils") == 0) ncase = 55;
       else if(strcmp(cbuf,"lcf") == 0) ncase = 56;
       else if(strcmp(cbuf,"hcf") == 0) ncase = 57;
       else if(strcmp(cbuf,"lcs") == 0) ncase = 58;
       else if(strcmp(cbuf,"hcs") == 0) ncase = 59;
       else if(strcmp(cbuf,"year") == 0) ncase = 60;
       else if(strcmp(cbuf,"day") == 0) ncase = 61;
       else if(strcmp(cbuf,"hour") == 0) ncase = 62;
       else if(strcmp(cbuf,"minute") == 0) ncase = 63;
       else if(strcmp(cbuf,"sec") == 0) ncase = 64;
       else if(strcmp(cbuf,"timbas") == 0) ncase = 65;
       else if(strcmp(cbuf,"trwf") == 0) ncase = 66;
       else if(strcmp(cbuf,"grnors") == 0) ncase = 67;
       else if(strcmp(cbuf,"grnofr") == 0) ncase = 68;
       else if(strcmp(cbuf,"grnlof") == 0) ncase = 69;
       else if(strcmp(cbuf,"gaps") == 0) ncase = 70;
       else if(strcmp(cbuf,"otrav") == 0) ncase = 71;
       else if(strcmp(cbuf,"d1") == 0) ncase = 72;
       else if(strcmp(cbuf,"f1") == 0) ncase = 73;
       else if(strcmp(cbuf,"d2") == 0) ncase = 74;
       else if(strcmp(cbuf,"f2") == 0) ncase = 75;
       else if(strcmp(cbuf,"ungpow") == 0) ncase = 76;
       else if(strcmp(cbuf,"unscale") == 0) ncase = 77;
       else if(strcmp(cbuf,"ntr") == 0) ncase = 78;
       else if(strcmp(cbuf,"mark") == 0) ncase = 79;
/*     else if(strncmp(cbuf,"numb",4) == 0) { */
/*       ncase = 1000 + atoi(cbuf+4);         */
/*     }                                      */
  
   return ncase;

}

/* --------------------------- */
double fromhead(segy tr, int k) {

       double dval=0.0;

       switch (k) {
   
         case -1: 
/*       null, name not found? */
         break;
         case 0:  
/*       null   do not read from header */ 
         break;
         case 1:
           dval = tr.tracl;
         break;
         case 2:
           dval = tr.tracr;
         break;
         case 3:
           dval = tr.fldr;
         break;
         case 4:
           dval = tr.tracf;
         break;
         case 5:
           dval = tr.ep;
         break;
         case 6:
           dval = tr.cdp;
         break;
         case 7:
           dval = tr.cdpt;
         break;
         case 8:
           dval = tr.trid;
         break;
         case 9:
           dval = tr.nvs;
         break;
         case 10:
           dval = tr.nhs;
         break;
         case 11:
           dval = tr.duse;
         break;
         case 12:
           dval = tr.offset;
         break;
         case 13:
           dval = tr.gelev;
         break;
         case 14:
           dval = tr.selev;
         break;
         case 15:
           dval = tr.sdepth;
         break;
         case 16:
           dval = tr.gdel;
         break;
         case 17:
           dval = tr.sdel;
         break;
         case 18:
           dval = tr.swdep;
         break;
         case 19:
           dval = tr.gwdep;
         break;
         case 20:
           dval = tr.scalel;
         break;
         case 21:
           dval = tr.scalco;
         break;
         case 22:
           dval = tr.sx;
         break;
         case 23:
           dval = tr.sy;
         break;
         case 24:
           dval = tr.gx;
         break;
         case 25:
           dval = tr.gy;
         break;
         case 26:
           dval = tr.counit;
         break;
         case 27:
           dval = tr.wevel;
         break;
         case 28:
           dval = tr.swevel;
         break;
         case 29:
           dval = tr.sut;
         break;
         case 30:
           dval = tr.gut;
         break;
         case 31:
           dval = tr.sstat;
         break;
         case 32:
           dval = tr.gstat;
         break;
         case 33:
           dval = tr.tstat;
         break;
         case 34:
           dval = tr.laga;
         break;
         case 35:
           dval = tr.lagb;
         break;
         case 36:
           dval = tr.delrt;
         break;
         case 37:
           dval = tr.muts;
         break;
         case 38:
           dval = tr.mute;
         break;
         case 39:
           dval = tr.ns;
         break;
         case 40:
           dval = tr.dt;
         break;
         case 41:
           dval = tr.gain;
         break;
         case 42:
           dval = tr.igc;
         break;
         case 43:
           dval = tr.igi;
         break;
         case 44:
           dval = tr.corr;
         break;
         case 45:
           dval = tr.sfs;
         break;
         case 46:
           dval = tr.sfe;
         break;
         case 47:
           dval = tr.slen;
         break;
         case 48:
           dval = tr.styp;
         break;
         case 49:
           dval = tr.stas;
         break;
         case 50:
           dval = tr.stae;
         break;
         case 51:
           dval = tr.tatyp;
         break;
         case 52:
           dval = tr.afilf;
         break;
         case 53:
           dval = tr.afils;
         break;
         case 54:
           dval = tr.nofilf;
         break;
         case 55:
           dval = tr.nofils;
         break;
         case 56:
           dval = tr.lcf;
         break;
         case 57:
           dval = tr.hcf;
         break;
         case 58:
           dval = tr.lcs;
         break;
         case 59:
           dval = tr.hcs;
         break;
         case 60:
           dval = tr.year;
         break;
         case 61:
           dval = tr.day;
         break;
         case 62:
           dval = tr.hour;
         break;
         case 63:
           dval = tr.minute;
         break;
         case 64:
           dval = tr.sec;
         break;
         case 65:
           dval = tr.timbas;
         break;
         case 66:
           dval = tr.trwf;
         break;
         case 67:
           dval = tr.grnors;
         break;
         case 68:
           dval = tr.grnofr;
         break;
         case 69:
           dval = tr.grnlof;
         break;
         case 70:
           dval = tr.gaps;
         break;
         case 71:
           dval = tr.otrav;
         break;
         case 72:
           dval = tr.d1;
         break;
         case 73:
           dval = tr.f1;
         break;
         case 74:
           dval = tr.d2;
         break;
         case 75:
           dval = tr.f2;
         break;
         case 76:
           dval = tr.ungpow;
         break;
         case 77:
           dval = tr.unscale;
         break;
         case 78:
           dval = tr.ntr;
         break;
         case 79:
           dval = tr.mark;
         break;
         case 80:
           dval = tr.shortpad;
         break;
  
/*      default:                           */
/*         err("unknown type %s", type);   */
/*      break;                             */
  
        } /* end of   switch */ 
        
      return (dval);
}

