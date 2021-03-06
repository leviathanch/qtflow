// blif2Verilog
//
// Revision 0, 2006-11-11: First release by R. Timothy Edwards.
// Revision 1, 2009-07-13: Minor cleanups by Philipp Klaus Krause.
// Revision 2, 2011-11-7: Added flag "-c" to maintain character case
// Revision 3, 2013-10-09: Changed input format from BDNET to BLIF
//
// This program is written in ISO C99.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <float.h>

#define	EXIT_SUCCESS	0
#define	EXIT_FAILURE	1
#define	EXIT_HELP	2
#define TRUE 		1
#define FALSE		0
#define NMOS		1
#define PMOS		0

#define LengthOfLine    	16384
#define LengthOfNodeName	512

/* getopt stuff */
extern	int	optind, getopt();
extern	char	*optarg;

#define INPUT	0
#define OUTPUT	1

struct Vect {
	struct Vect *next;
	char *name;
	char direction;		/* INPUT or OUTPUT */
	int Max;
};

void ReadNetlistAndConvert(FILE *, FILE *, int, int);
void CleanupString(char text[]);
void ToLowerCase( char *text);
float getnumber(char *strpntbegin);
int loc_getline( char s[], int lim, FILE *fp);
void helpmessage();
int ParseNumber( char *test);
struct Vect *VectorAlloc(void);

char *VddNet = NULL;
char *GndNet = NULL;

int main ( int argc, char *argv[])
{
	FILE *NET1 = NULL, *NET2, *OUT;
	struct Resistor *ResistorData;
	int i,AllMatched,NetsEqual,ImplicitPower,MaintainCase;

	char *Net1name = NULL;

	VddNet = strdup("VDD");
	GndNet = strdup("VSS");
	
	ImplicitPower=TRUE;
	MaintainCase=FALSE;
        while( (i = getopt( argc, argv, "pchHv:g:" )) != EOF ) {
	   switch( i ) {
	   case 'p':
	       ImplicitPower=FALSE;
	       break;
	   case 'c':
	       MaintainCase=TRUE;
	       break;
	   case 'h':
	   case 'H':
	       helpmessage();
	       break;
	   case 'v':
	       free(VddNet);
	       VddNet = strdup(optarg);
	       break;
	   case 'g':
	       free(GndNet);
	       GndNet = strdup(optarg);
	       break;
	   default:
	       fprintf(stderr,"\nbad switch %d\n", i );
	       helpmessage();
	       break;
	   }
        }

        if( optind < argc )	{
	   Net1name = strdup(argv[optind]);
	   optind++;
	}
	else	{
	   fprintf(stderr,"Couldn't find a filename as input\n");
	   exit(EXIT_FAILURE);
	}
        optind++;
	if (Net1name)
	    NET1=fopen(Net1name,"r");
	if (NET1 == NULL ) {
		fprintf(stderr,"Couldn't open %s for read\n",Net1name);
		exit(EXIT_FAILURE);
	}
	OUT=stdout;
	ReadNetlistAndConvert(NET1,OUT,ImplicitPower,MaintainCase);
        return 0;


}


struct GateList {
   struct GateList *next;
   char *gatename;
   int gatecount;
};


/*--------------------------------------------------------------*/
/*C *Alloc - Allocates memory for linked list elements		*/
/*								*/
/*         ARGS: 
        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/
void ReadNetlistAndConvert(FILE *NETFILE, FILE *OUT, int ImplicitPower, int MaintainCase)
{
	struct Vect *Vector, *VectorPresent;
	struct GateList *glist;
	struct GateList *gl;
	int i, Found, NumberOfInputs, NumberOfOutputs;
	int First, VectorIndex, ItIsAnInput, ItIsAnOutput, PrintIt;

        char line[LengthOfLine];

	char *Weirdpnt, *lptr;
	char *allinputs = NULL;
	char *alloutputs = NULL;
	char **InputNodes;
	char **OutputNodes;
	char InputName[LengthOfNodeName];
	char OutputName[LengthOfNodeName];
        char MainSubcktName[LengthOfNodeName];
	char InstanceName[LengthOfNodeName];
	char InstancePortName[LengthOfNodeName];
	char InstancePortWire[LengthOfNodeName];
	char dum[LengthOfNodeName];

	glist = NULL;

	NumberOfOutputs = 0;
	NumberOfInputs = 0;

	InputNodes = (char **)malloc(sizeof(char *));
	OutputNodes = (char **)malloc(sizeof(char *));

	/* Read in line by line */

	First = TRUE;
	Vector = VectorAlloc();
	Vector->next = NULL;
        while (loc_getline(line, sizeof(line), NETFILE) > 0) {
	   lptr = line;
	   while (isspace(*lptr)) lptr++;
	   if (strstr(lptr, ".model") != NULL) {
              if (sscanf(lptr, ".model %s", MainSubcktName) == 1) {
	         CleanupString(MainSubcktName);
	         fprintf(OUT, "module %s (", MainSubcktName);
	         if (ImplicitPower) fprintf(OUT, " %s, %s, ", GndNet, VddNet); 
	      }
	   }
	   if (strstr(lptr, ".inputs") != NULL) {
	      allinputs = (char *)malloc(1);
	      allinputs[0] = '\0';
	      while (!isspace(*lptr)) lptr++;
	      while (isspace(*lptr)) lptr++;
	      while (1) {
                 if (sscanf(lptr, "%s", InputName) == 1) {
	            PrintIt = TRUE;
	            CleanupString(InputName);
		    InputNodes[NumberOfInputs] = strdup(InputName);
	            if ((Weirdpnt = strchr(InputName,'[')) != NULL) {
	               PrintIt = FALSE;
	               VectorIndex = ParseNumber(Weirdpnt); // This one needs to cut off [..]
	               VectorPresent = Vector;
	               Found = FALSE;
	               while (VectorPresent->next != NULL && !Found) {
	                  if (strcmp(VectorPresent->name, InputName) == 0) {
	                     VectorPresent->Max = (VectorPresent->Max > VectorIndex) ?
					VectorPresent->Max : VectorIndex;
	                     Found = TRUE;
	                  }
	                  VectorPresent = VectorPresent->next; 
	               }
	               if (!Found) {
			  VectorPresent->name = strdup(InputName);
			  VectorPresent->direction = INPUT;
	                  VectorPresent->Max = VectorIndex;
	                  VectorPresent->next = VectorAlloc();
	                  VectorPresent->next->next = NULL;
	               }
	            }
	            if (PrintIt || !Found) {	// Should print vectors in module statement
	               if (First) {
	                  fprintf(OUT, "%s", InputName);
	                  First = FALSE;
	               }
	               else fprintf(OUT, ", %s", InputName);
	            }
	            if (PrintIt) {		//Should not print vectors now
		       allinputs = (char *)realloc(allinputs,
				strlen(allinputs) + strlen(InputName) + 9);
	               strcat(allinputs, "input ");
	               strcat(allinputs, InputName);
	               strcat(allinputs, ";\n");
	            }
	            NumberOfInputs++;
		    InputNodes = (char **)realloc(InputNodes, (NumberOfInputs + 1) * sizeof(char *));
		    while (!isspace(*lptr)) lptr++;
		    while (isspace(*lptr)) lptr++;
		    if (*lptr == '\\') {
	               if (loc_getline(line, sizeof(line), NETFILE) <= 1)
			   break;
		       else {
			   lptr = line;
			   while (isspace(*lptr)) lptr++;
		       }
		    }
		    if (*lptr == '\n' || *lptr == '\0') break;
	         }
		 else break;
	      }
	   }
	   if (strstr(lptr, ".outputs") != NULL) {
	      alloutputs = (char *)malloc(1);
	      alloutputs[0] = '\0';
	      while (!isspace(*lptr)) lptr++;
	      while (isspace(*lptr)) lptr++;
	      while (1) {
                 if (sscanf(lptr, "%s", OutputName) == 1) {
	            PrintIt = TRUE;
	            CleanupString(OutputName);
		    OutputNodes[NumberOfOutputs] = strdup(OutputName);
	            if ((Weirdpnt = strchr(OutputName,'[')) != NULL) {
	               PrintIt = FALSE;
	               VectorIndex = ParseNumber(Weirdpnt);	// This one needs to cut off [..]
	               VectorPresent = Vector;
	               Found = FALSE;
	               while (VectorPresent->next != NULL && !Found) {
	                  if (strcmp(VectorPresent->name, OutputName) == 0) {
	                     VectorPresent->Max = (VectorPresent->Max > VectorIndex) ?
					VectorPresent->Max : VectorIndex;
	                     Found = TRUE;
	                  }
	                  VectorPresent = VectorPresent->next; 
	               }
	               if (!Found) {
			  VectorPresent->name = strdup(OutputName);
	                  VectorPresent->direction = OUTPUT;
	                  VectorPresent->Max = VectorIndex;
	                  VectorPresent->next = VectorAlloc();
	                  VectorPresent->next->next = NULL;
	               }
	            }
	            if (PrintIt || !Found) {
	               if (First) {
	                  fprintf(OUT, "%s", OutputName);
	                  First = FALSE;
	               }
	               else fprintf(OUT, ", %s", OutputName);
	            }
	            if (PrintIt) {
		       alloutputs = (char *)realloc(alloutputs,
				strlen(alloutputs) + strlen(OutputName) + 10);
	               strcat(alloutputs, "output ");
	               strcat(alloutputs, OutputName);
	               strcat(alloutputs, ";\n");
	            }
	            NumberOfOutputs++;
		    OutputNodes = (char **)realloc(OutputNodes, (NumberOfOutputs + 1) * sizeof(char *));
		    while (!isspace(*lptr)) lptr++;
		    while (isspace(*lptr)) lptr++;
		    if (*lptr == '\\') {
		       if (loc_getline(line, sizeof(line), NETFILE) <= 1)
			  break;
		       else {
			  lptr = line;
			  while (isspace(*lptr)) lptr++;
		       }
		    }
		    if (*lptr == '\n' || *lptr == '\0') break;
	         }
		 else break;
	      }
	      fprintf(OUT,");\n\n");
	      if (ImplicitPower)
		  fprintf(OUT, "input %s, %s;\n", GndNet, VddNet);

	      if (allinputs) fprintf(OUT, "%s", allinputs);
	      if (alloutputs) fprintf(OUT, "%s", alloutputs);

	      VectorPresent = Vector;
	      while (VectorPresent->next != NULL) {
	         fprintf(OUT, "%s [%d:0] %s;\n",
				(VectorPresent->direction == INPUT) ?
				"input" : "output",
				VectorPresent->Max,
				VectorPresent->name);
	         VectorPresent = VectorPresent->next;
	      }
	      fprintf(OUT, "\n");
	      if (!ImplicitPower) {
		 fprintf(OUT, "wire %s = 1'b1;\n", VddNet);
		 fprintf(OUT, "wire %s = 1'b0;\n", GndNet);
		 fprintf(OUT, "\n");
	      }
	   }
	   if (strstr(lptr, ".end") != NULL) {
              fprintf(OUT, "endmodule\n");
           }
	   if (strstr(lptr,".gate") != NULL || strstr(lptr, ".subckt") != NULL) {
	      if (sscanf(lptr, ".%*s %s", InstanceName) == 1) {
	         CleanupString(InstanceName);

	         if (!MaintainCase) ToLowerCase(InstanceName);

		 for (gl = glist; gl; gl = gl->next) {
		    if (!strcmp(gl->gatename, InstanceName)) {
		       gl->gatecount++;
		       break;
		    }
		 }
		 if (gl == NULL) {
		    gl = (struct GateList *)malloc(sizeof(struct GateList));
		    gl->gatename = strdup(InstanceName);
		    gl->gatecount = 1;
		    gl->next = glist;
		    glist = gl;
		 }

	         fprintf(OUT, "\t%s %s_%d ( ", gl->gatename, gl->gatename, gl->gatecount);
	         First = TRUE;
	         if (ImplicitPower) fprintf(OUT, ".%s(%s), .%s(%s), ",
			GndNet, GndNet, VddNet, VddNet); 
		
	         while (!isspace(*lptr)) lptr++;
	         while (isspace(*lptr)) lptr++;
	         while (!isspace(*lptr)) lptr++;
	         while (isspace(*lptr)) lptr++;
		 while (1) {
		    char *eptr;
		    eptr = strchr(lptr, '=');
		    if (eptr == NULL) break;
		    *eptr = '\0';
	            if (sscanf(lptr, "%s", InstancePortName) != 1) break;
		    lptr = eptr + 1;
	            if (sscanf(lptr, "%s", InstancePortWire) != 1) break;
	            CleanupString(InstancePortName);
	            CleanupString(InstancePortWire);

	            ItIsAnInput = FALSE;
	            ItIsAnOutput = FALSE;
	            for (i = 0; i < NumberOfInputs; i++) {
	               if (!strcmp(InstancePortWire, InputNodes[i])) {
	                  ItIsAnInput = TRUE;
	                  strcpy(InstancePortWire, InputNodes[i]);
	                  Weirdpnt = strchr(InstancePortWire, ']');
	                  if(Weirdpnt != NULL) *(Weirdpnt + 1) = '\0';
	               }
	            }
	            for (i = 0; i < NumberOfOutputs; i++) {
	               if (!strcmp(InstancePortWire, OutputNodes[i])) {
	                  ItIsAnOutput = TRUE;
	                  strcpy(InstancePortWire, OutputNodes[i]);
	                  Weirdpnt = strchr(InstancePortWire, ']');
	                  if (Weirdpnt != NULL) *(Weirdpnt + 1) = '\0';
	               }
	            }
	            if (!ItIsAnInput && !ItIsAnOutput) {
	               if ((Weirdpnt = strchr(InstancePortWire,'[')) != NULL) {
	                  *Weirdpnt = '_';
	                  if ((Weirdpnt = strchr(InstancePortWire,']')) != NULL) 
	                     *Weirdpnt = '_';
	               }
	               while ((Weirdpnt = strchr(InstancePortWire, '$')) != NULL) {
	                  *Weirdpnt = '_';
	                  Weirdpnt++;
	               }
	            }

	                 
	            if (InstancePortWire[0] <= '9' && InstancePortWire[0] >= '0') {
	               strcpy(dum, "N_");
	               strcat(dum, InstancePortWire);
	               strcpy(InstancePortWire, dum);
	            }
	            if (First) {
	               fprintf(OUT, ".%s(%s)", InstancePortName, InstancePortWire);
	               First = FALSE;
	            }
	            else fprintf(OUT, ", .%s(%s)", InstancePortName, InstancePortWire);

		    while (!isspace(*lptr)) lptr++;
		    while (isspace(*lptr)) lptr++;
		    if (*lptr == '\\') {
			if (loc_getline(line, sizeof(line), NETFILE) <= 1) break;
			lptr = line;
		    }
		    else if (*lptr == '\n') break;
	         }
	         fprintf(OUT, " );\n");
	      }
	   } 
	}
}


int ParseNumber( char *text)
{
	char *begin, *end;
// Assumes *text is a '['
	begin=(text+1);
	end=strchr(begin,']');
	*end='\0';
	*text='\0';
	return atoi(begin);
}
	

	

void ToLowerCase( char *text)
{
        int i=0;

        while( text[i] != '\0' ) {
           text[i]=tolower(text[i]);
           i++;
        }
}


void CleanupString(char text[LengthOfNodeName])
{
	int i;
	char *CitationPnt, *Weirdpnt;
	
	CitationPnt=strchr(text,'"');
	if( CitationPnt != NULL) {
	   i=0;
	   while( CitationPnt[i+1] != '"' ) {
	      CitationPnt[i]=CitationPnt[i+1];
	      i+=1;
	   }
	   CitationPnt[i]='\0';
	}

	// Convert angle brackets to square brackets if they
	// occur at the end of a name;  otherwise, convert
	// them to underscores

	while ((Weirdpnt = strchr(text,'<')) != NULL) {
	   char *eptr;

	   eptr = strchr(Weirdpnt, '>');

	   if (eptr == NULL) {
	      *Weirdpnt = '_';
	   }
	   else {
	      if (*(eptr + 1) == '\0') {
		 *Weirdpnt = '[';
		 *eptr = ']';
	      }
	      else {
		 *Weirdpnt = '_';
		 *eptr = '_';
	      }
	   }
	}

	// Disallow characters '.' and ':' in node names
	
	while ((Weirdpnt=strchr(text,'.')) != NULL)
	   *Weirdpnt='_';
	while ((Weirdpnt=strchr(text,':')) != NULL)
	   *Weirdpnt='_';
}


/*--------------------------------------------------------------*/
/*C getnumber - gets number pointed by strpntbegin		*/
/*								*/
/*         ARGS: strpntbegin - number expected after '='        */ 
/*        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/
float getnumber(char *strpntbegin)
{       int i;
        char *strpnt,magn1,magn2;
        float number;

        strpnt=strpntbegin;
        strpnt=strchr(strpntbegin,'=');
        if(strpnt == NULL) {
           fprintf(stderr,"Error: getnumber: Didn't find '=' in string "
                               "%s\n",strpntbegin);
           return DBL_MAX;
        }
        strpnt=strpnt+1;
        
	if(sscanf(strpnt,"%f%c%c",&number,&magn1, &magn2)!=3) {
            if(sscanf(strpnt,"%f%c",&number,&magn1)!=2) {
               fprintf(stderr,"Error: getnumber : Couldn't read number in "
                      "%s %s\n",strpntbegin,strpnt);
               return DBL_MAX;
            }
        }
/*if(*strpntbegin =='m') */
        switch( magn1 ) {
        case 'f':
           number *= 1e-15;
           break;          
        case 'p':
           number *= 1e-12;
           break;          
        case 'n':
           number *= 1e-9;
           break;          
        case 'u':
           number *= 1e-6;
           break;          
        case 'm':
           if (magn2 == 'e') number *= 1e6;
           else number *= 1e-3;
           break;          
        case 'k':
           number *= 1e3;
           break;          
        case 'g':
           number *= 1e9;
           break;
        case ' ':
           default:
           return number;
        }
        return number;
}          


/*--------------------------------------------------------------*/
/*C loc_getline: read a line, return length		*/
/*								*/
/*         ARGS: 
        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/
int loc_getline( char s[], int lim, FILE *fp)
{
	int c, i;
	
	i=0;
	while(--lim > 0 && (c=getc(fp)) != EOF && c != '\n')
		s[i++] = c;
	if (c == '\n')
		s[i++] = c;
	s[i] = '\0';
	if ( c == EOF ) i=0; 
	return i;
}


struct Vect *VectorAlloc(void)
{
	return (struct Vect *) malloc(sizeof(struct Vect));
}


/*--------------------------------------------------------------*/
/*C helpmessage - tell user how to use the program		*/
/*								*/
/*         ARGS: 
        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/


void helpmessage()
{

    fprintf(stderr,"blif2Verilog [-options] netlist \n");
    fprintf(stderr,"\n");
    fprintf(stderr,"blif2Verilog converts a netlist in blif format \n");
    fprintf(stderr,"to Verilog format. Output on stdout\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"option, -h this message\n");    
    fprintf(stderr,"option, -p means: don't add power nodes to instances\n");
    fprintf(stderr,"        only nodes present in the .gate statement used\n");

    exit( EXIT_HELP );	
} /* helpmessage() */


