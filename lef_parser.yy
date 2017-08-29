%define api.prefix {lef}
%defines "lef.yy.h"
%error-verbose
%language "c++"
%glr-parser
%debug
%token-table
%define "parser_class_name" {LEFParser}

%code requires {
	namespace lef {
		class LEFData;
	}
}

%param {class lef::LEFData *lefdata}

%{
#include <iostream>
#include <string>
#include <QString>
#include "lefdata.h"
#include "lefscanner.h"

#define leflex (lefdata->getLexer())->leflex
#define leflineno (int)(lefdata->getLexer())->lineno()

%}

%union {
	int v_int;
	std::string* v_str;
	double v_double;
}

%token <v_int> INTEGER
%token <v_str> STRING
%token <v_double> DOUBLE

%token VERSION
%token NAMESCASESENSITIVE
%token BUSBITCHARS
%token DIVIDERCHAR
%token UNITS
%token END
%token DATABASE
%token MICRONS
%token USEMINSPACING
%token OBS
%token PIN
%token CLEARANCEMEASURE
%token MANUFACTURINGGRID
%token LAYER
%token TYPE
%token SPACING
%token DIRECTION
%token PITCH
%token OFFSET
%token WIDTH
%token RESISTANCE
%token CAPACITANCE
%token VIA
%token RECT
%token VIARULE
%token TO
%token BY
%token OVERHANG
%token METALOVERHANG

%start lef_file

%%

lef_file: options layers vias viarules;

options: option | option options;
option:
	  version
	| cases
	| bitchars
	| dividechar
	| units
	| useminamespacing
	| clearensmeasure
	| manufacturinggrid
	;

version: VERSION DOUBLE;
cases: NAMESCASESENSITIVE STRING;
bitchars: BUSBITCHARS STRING;
dividechar: DIVIDERCHAR STRING;
units: UNITS database_list END UNITS;
database_list: DATABASE MICRONS INTEGER;
useminamespacing: USEMINSPACING OBS STRING | USEMINSPACING PIN STRING;
clearensmeasure: CLEARANCEMEASURE STRING;
manufacturinggrid: MANUFACTURINGGRID DOUBLE;

layers: layer | layers layer;
layer: layer_name layer_options END STRING;
layer_name: LAYER STRING;
layer_type: TYPE STRING;
layer_spacing: SPACING DOUBLE;

layer_options: layer_option | layer_options layer_option;
layer_option:
	  layer_type
	| layer_spacing
	| DIRECTION STRING
	| PITCH DOUBLE
	| PITCH INTEGER
	| OFFSET DOUBLE
	| OFFSET INTEGER
	| WIDTH DOUBLE
	| WIDTH INTEGER
	| RESISTANCE STRING DOUBLE
	| RESISTANCE STRING INTEGER
	| CAPACITANCE STRING DOUBLE
	| CAPACITANCE STRING INTEGER
	;

vias: via | vias via;
via: via_name via_layers END STRING;
via_name: VIA STRING STRING;
via_layers: via_layer | via_layers via_layer;
via_layer: LAYER STRING via_rects;
via_rects: via_rect | via_rects via_rect;
via_rect: RECT DOUBLE DOUBLE DOUBLE DOUBLE;

viarules: viarule | viarules viarule;
viarule: viarule_name viarule_layers END STRING;
viarule_name: VIARULE STRING STRING;
viarule_layers: viarule_layer | viarule_layers viarule_layer;
viarule_layer: viarule_layer_name viarule_layer_options;
viarule_layer_name: LAYER STRING;
viarule_layer_options: viarule_layer_option | viarule_layer_options viarule_layer_option;

viarule_layer_option:
	  viarule_layer_option_direction
	| viarule_layer_option_width
	| viarule_layer_option_overhang
	| viarule_layer_option_metaloverhang
	| viarule_layer_option_rect
	| viarule_layer_option_spacing
	;

viarule_layer_option_direction: DIRECTION STRING;
viarule_layer_option_width: WIDTH DOUBLE TO INTEGER | WIDTH DOUBLE TO DOUBLE;
viarule_layer_option_overhang: OVERHANG INTEGER | OVERHANG DOUBLE;
viarule_layer_option_metaloverhang: METALOVERHANG INTEGER | METALOVERHANG DOUBLE;
viarule_layer_option_rect: RECT DOUBLE DOUBLE DOUBLE DOUBLE;
viarule_layer_option_spacing: SPACING INTEGER BY INTEGER | SPACING DOUBLE BY DOUBLE;

%%

void lef::LEFParser::error(const std::string &s) {
	std::cout << "Error message: " << s << " on line " << leflineno << std::endl;
}
