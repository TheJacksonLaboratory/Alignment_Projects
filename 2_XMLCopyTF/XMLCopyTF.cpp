//
// Copy tforms from file2=TFormTable.txt into file1=TrakEM2.xml.
//

#include	"Cmdline.h"
#include	"CRegexID.h"
#include	"File.h"
#include	"TrakEM2_UTL.h"
#include	"CTForm.h"

#include	<map>
#include	<set>
using namespace std;


/* --------------------------------------------------------------- */
/* Macros -------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Types --------------------------------------------------------- */
/* --------------------------------------------------------------- */

// ----------------------------------------

// Map {z,id} <-> TForm

class MZID {

public:
	int	z, id;

public:
	MZID() {};

	bool operator < (const MZID &rhs) const
		{
			if( z < rhs.z )
				return true;
			if( z > rhs.z )
				return false;

			return id < rhs.id;
		};

	bool operator == (const MZID &rhs) const
		{return z == rhs.z && id == rhs.id;};
};

/* --------------------------------------------------------------- */
/* CArgs_xml ----------------------------------------------------- */
/* --------------------------------------------------------------- */

class CArgs_xml {

private:
	// re_id used to extract tile id from image name.
	// "/N" used for EM projects, "_N_" for APIG images.
	CRegexID	re_id;

public:
	char	*xmlfile,
			*tblfile;

public:
	CArgs_xml()
	{
		xmlfile	= NULL;
		tblfile	= NULL;
	};

	void SetCmdLine( int argc, char* argv[] );

	int IDFromPatch( TiXmlElement *p );
};

/* --------------------------------------------------------------- */
/* Statics ------------------------------------------------------- */
/* --------------------------------------------------------------- */

static CArgs_xml		gArgs;
static FILE*			flog = NULL;
static map<MZID, TForm>	M;
static set<int>			Z;






/* --------------------------------------------------------------- */
/* SetCmdLine ---------------------------------------------------- */
/* --------------------------------------------------------------- */

void CArgs_xml::SetCmdLine( int argc, char* argv[] )
{
// start log

	flog = FileOpenOrDie( "XMLCopyTF.log", "w" );

// log start time

	time_t	t0 = time( NULL );
	char	atime[32];

	strcpy( atime, ctime( &t0 ) );
	atime[24] = '\0';	// remove the newline

	fprintf( flog, "Start: %s ", atime );

// parse command line args

	char	*pat;

	re_id.Set( "_N_" );

	if( argc < 3 ) {
		printf(
		"Usage: XMLCopyTF <xml-file> <tbl-file>.\n" );
		exit( 42 );
	}

	for( int i = 1; i < argc; ++i ) {

		// echo to log
		fprintf( flog, "%s ", argv[i] );

		if( argv[i][0] != '-' ) {

			if( !xmlfile )
				xmlfile = argv[i];
			else
				tblfile = argv[i];
		}
		else if( GetArgStr( pat, "-p", argv[i] ) )
			re_id.Set( pat );
	}

	fprintf( flog, "\n" );

	re_id.Compile( flog );

	fflush( flog );
}

/* -------------------------------------------------------------- */
/* IDFromPatch -------------------------------------------------- */
/* -------------------------------------------------------------- */

int CArgs_xml::IDFromPatch( TiXmlElement *p )
{
	const char	*name = p->Attribute( "title" );
	int			id;

	if( !re_id.Decode( id, name ) ) {
		printf( "No tile-id found in '%s'.\n", name );
		exit( 42 );
	}

	return id;
}

/* --------------------------------------------------------------- */
/* LoadTForms ---------------------------------------------------- */
/* --------------------------------------------------------------- */

static void LoadTForms()
{
	FILE		*f		= FileOpenOrDie( gArgs.tblfile, "r", flog );
	CLineScan	LS;
	int			lastz	= -1;

	for(;;) {

		if( LS.Get( f ) <= 0 )
			break;

		MZID	R;
		TForm	T;
		int		rgn;

		sscanf( LS.line, "%d\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf",
		&R.z, &R.id, &rgn,
		&T.t[0], &T.t[1], &T.t[2],
		&T.t[3], &T.t[4], &T.t[5] );

		if( rgn != 1 )
			continue;

		Z.insert( R.z );

		M[R] = T;
	}

	fclose( f );
}

/* --------------------------------------------------------------- */
/* Update -------------------------------------------------------- */
/* --------------------------------------------------------------- */

static void Update()
{
/* ------------- */
/* Load document */
/* ------------- */

	TiXmlDocument	doc( gArgs.xmlfile );

	if( !doc.LoadFile() ) {
		fprintf( flog,
		"Could not open XML file [%s].\n", gArgs.xmlfile );
		exit( 42 );
	}

/* ---------------- */
/* Verify <trakem2> */
/* ---------------- */

	TiXmlHandle		hdoc( &doc );
	TiXmlElement	*layer;

	if( !doc.FirstChild() ) {
		fprintf( flog,
		"No trakEM2 node [%s].\n", gArgs.xmlfile );
		exit( 42 );
	}

	layer = hdoc.FirstChild( "trakem2" )
				.FirstChild( "t2_layer_set" )
				.FirstChild( "t2_layer" )
				.ToElement();

	if( !layer ) {
		fprintf( flog,
		"No t2_layer [%s].\n", gArgs.xmlfile );
		exit( 42 );
	}

/* ------------------------ */
/* Copy matching transforms */
/* ------------------------ */

	for( ; layer; layer = layer->NextSiblingElement() ) {

		MZID	key;

		key.z = atoi( layer->Attribute( "z" ) );

		if( Z.find( key.z ) == Z.end() )
			continue;

		TiXmlElement	*next = NULL;

		for(
			TiXmlElement *p = layer->FirstChildElement( "t2_patch" );
			p;
			p = next ) {

			next = p->NextSiblingElement();

			key.id = gArgs.IDFromPatch( p );

			map<MZID, TForm>::iterator	it = M.find( key );

			if( it == M.end() ) {
				layer->RemoveChild( p );
				continue;
			}

			const double	*t = it->second.t;
			char			buf[256];

			sprintf( buf, "matrix(%f,%f,%f,%f,%f,%f)",
			t[0], t[3], t[1], t[4], t[2], t[5] );

			p->SetAttribute( "transform", buf );
		}
	}

/* ---- */
/* Save */
/* ---- */

	doc.SaveFile( "xmltmp.txt" );

/* ----------------- */
/* Copy !DOCTYPE tag */
/* ----------------- */

	CopyDTD( gArgs.xmlfile, "xmltmp.txt" );
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

/* ------------------------------------- */
/* Load lists of TForms and affected Z's */
/* ------------------------------------- */

	LoadTForms();

/* ------------- */
/* Write new xml */
/* ------------- */

	Update();

/* ---- */
/* Done */
/* ---- */

exit:
	fprintf( flog, "\n" );
	fclose( flog );

	return 0;
}


