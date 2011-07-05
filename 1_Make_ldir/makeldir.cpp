//
// Make ldir file with entries:
//
// DIR z string
//
// Where the string is a file_path up to and including the pattern
// command line argument. Note that the pattern is applied to the
// filename part of the full path string.
//

#include	"Cmdline.h"
#include	"File.h"

#include	"tinyxml.h"

#include	<set>
using namespace std;


/* --------------------------------------------------------------- */
/* Macros -------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Types --------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* CArgs_ldir ---------------------------------------------------- */
/* --------------------------------------------------------------- */

class CArgs_ldir {

public:
	char	*infile,
			*pat;
	int		zmin, zmax;

public:
	CArgs_ldir()
	{
		infile	= NULL;
		pat		= NULL;
		zmin	= 0;
		zmax	= 32768;
	};

	void SetCmdLine( int argc, char* argv[] );
};

/* --------------------------------------------------------------- */
/* Statics ------------------------------------------------------- */
/* --------------------------------------------------------------- */

static CArgs_ldir	gArgs;
static FILE*		flog = NULL;






/* --------------------------------------------------------------- */
/* SetCmdLine ---------------------------------------------------- */
/* --------------------------------------------------------------- */

void CArgs_ldir::SetCmdLine( int argc, char* argv[] )
{
// start log

	flog = FileOpenOrDie( "makeldir.log", "w" );

// log start time

	time_t	t0 = time( NULL );
	char	atime[32];

	strcpy( atime, ctime( &t0 ) );
	atime[24] = '\0';	// remove the newline

	fprintf( flog, "Start: %s ", atime );

// parse command line args

	if( argc < 3 ) {
		printf( "Usage: makeldir <xml-file> -ppat [options].\n" );
		printf( "Suggested patterns: <optical> -p_  <EM> -psq_\n" );
		exit( 42 );
	}

	for( int i = 1; i < argc; ++i ) {

		// echo to log
		fprintf( flog, "%s ", argv[i] );

		if( argv[i][0] != '-' )
			infile = argv[i];
		else if( GetArgStr( pat, "-p", argv[i] ) )
			;
		else if( GetArg( &zmin, "-zmin=%d", argv[i] ) )
			;
		else if( GetArg( &zmax, "-zmax=%d", argv[i] ) )
			;
		else {
			printf( "Did not understand option '%s'.\n", argv[i] );
			exit( 42 );
		}
	}

	fprintf( flog, "\n\n" );
	fflush( flog );
}

/* --------------------------------------------------------------- */
/* ParseTrakEM2 -------------------------------------------------- */
/* --------------------------------------------------------------- */

static void ParseTrakEM2()
{
/* ------------- */
/* Load document */
/* ------------- */

	TiXmlDocument	doc( gArgs.infile );
	bool			loadOK = doc.LoadFile();

	if( !loadOK ) {
		fprintf( flog,
		"Could not open XML file [%s].\n", gArgs.infile );
		exit( 42 );
	}

/* ---------------- */
/* Verify <trakem2> */
/* ---------------- */

	TiXmlHandle		hDoc( &doc );
	TiXmlElement*	layer;

	if( !doc.FirstChild() ) {
		fprintf( flog, "No trakEM2 node.\n" );
		exit( 42 );
	}

	layer = hDoc.FirstChild( "trakem2" )
				.FirstChild( "t2_layer_set" )
				.FirstChild( "t2_layer" )
				.ToElement();

	if( !layer ) {
		fprintf( flog, "No first trakEM2 child.\n" );
		exit( 42 );
	}

/* --------- */
/* Open file */
/* --------- */

	FILE	*fldir = FileOpenOrDie( "ldir", "w", flog );

/* -------------- */
/* For each layer */
/* -------------- */

	for( ; layer; layer = layer->NextSiblingElement() ) {

		/* ----------------- */
		/* Layer-level stuff */
		/* ----------------- */

		int	z = atoi( layer->Attribute( "z" ) );

		if( z > gArgs.zmax )
			break;

		if( z < gArgs.zmin )
			continue;

		/* ----------------------------------------- */
		/* Collect all unique folders for this layer */
		/* ----------------------------------------- */

		set<string>		S;
		TiXmlElement*	p;

		for(
			p = layer->FirstChildElement( "t2_patch" );
			p;
			p = p->NextSiblingElement() ) {

			char		buf[2048];
			const char*	name = p->Attribute( "file_path" );
			const char*	slsh = strrchr( name, '/' );
			const char*	term = strstr( slsh, gArgs.pat );

			sprintf( buf, "%.*s%s", term - name, name, gArgs.pat );
			S.insert( string( buf ) );
		}

		/* ------------------ */
		/* And write them out */
		/* ------------------ */

		set<string>::iterator	it;

		for( it = S.begin(); it != S.end(); ++it )
			fprintf( fldir, "DIR %d %s\n", z, it->c_str() );
	}

/* ---- */
/* Done */
/* ---- */

	fclose( fldir );
}

/* --------------------------------------------------------------- */
/* main ---------------------------------------------------------- */
/* --------------------------------------------------------------- */

int main( int argc, char* argv[] )
{
/* ------------------ */
/* Parse command line */
/* ------------------ */

	gArgs.SetCmdLine( argc, argv );

/* ---------------- */
/* Read source file */
/* ---------------- */

	ParseTrakEM2();

/* ---- */
/* Done */
/* ---- */

	fprintf( flog, "\n" );
	fclose( flog );

	return 0;
}


