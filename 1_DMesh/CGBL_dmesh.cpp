

#include	"CGBL_dmesh.h"

#include	"Cmdline.h"
#include	"File.h"
#include	"Debug.h"


/* --------------------------------------------------------------- */
/* Constants ----------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Macros -------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Types --------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Globals ------------------------------------------------------- */
/* --------------------------------------------------------------- */

CGBL_dmesh	GBL;

/* --------------------------------------------------------------- */
/* Statics ------------------------------------------------------- */
/* --------------------------------------------------------------- */






/* --------------------------------------------------------------- */
/* Object management --------------------------------------------- */
/* --------------------------------------------------------------- */

CGBL_dmesh::CGBL_dmesh()
{
	_arg.SCALE			= 999.0;
	_arg.XSCALE			= 999.0;
	_arg.YSCALE			= 999.0;
	_arg.SKEW			= 999.0;
	_arg.ima			= NULL;
	_arg.imb			= NULL;
	_arg.MODE			= 0;

	arg.CTR				= 999.0;
	arg.fma				= NULL;
	arg.fmb				= NULL;
	arg.Transpose		= false;
	arg.WithinSection	= false;
	arg.Verbose			= false;
	arg.NoFolds			= false;
	arg.SingleFold		= false;
	arg.Heatmap			= false;

	A.layer	= 0;
	A.tile	= 0;

	B.layer	= 0;
	B.tile	= 0;
}

/* --------------------------------------------------------------- */
/* SetCmdLine ---------------------------------------------------- */
/* --------------------------------------------------------------- */

bool CGBL_dmesh::SetCmdLine( int argc, char* argv[] )
{
// Parse args

	char			*key;
	vector<double>	vD;

	for( int i = 1; i < argc; ++i ) {

		if( argv[i][0] != '-' )
			key = argv[i];
		else if( GetArgList( vD, "-Tdfm=", argv[i] ) ) {

			if( 6 == vD.size() )
				_arg.Tdfm.push_back( TForm( &vD[0] ) );
			else {
				printf(
				"main: WARNING: Bad format in -Tdfm [%s].\n",
				argv[i] );
			}
		}
		else if( GetArgList( vD, "-Tab=", argv[i] ) ) {

			if( 6 == vD.size() )
				_arg.Tab.push_back( TForm( &vD[0] ) );
			else {
				printf(
				"main: WARNING: Bad format in -Tab [%s].\n",
				argv[i] );
			}
		}
		else if( GetArg( &_arg.SCALE, "-SCALE=%lf", argv[i] ) )
			;
		else if( GetArg( &_arg.XSCALE, "-XSCALE=%lf", argv[i] ) )
			;
		else if( GetArg( &_arg.YSCALE, "-YSCALE=%lf", argv[i] ) )
			;
		else if( GetArg( &_arg.SKEW, "-SKEW=%lf", argv[i] ) )
			;
		else if( GetArgStr( _arg.ima, "-ima=", argv[i] ) )
			;
		else if( GetArgStr( _arg.imb, "-imb=", argv[i] ) )
			;
		else if( GetArg( &_arg.MODE, "-MODE=%c", argv[i] ) )
			;
		else if( GetArg( &arg.CTR, "-CTR=%lf", argv[i] ) )
			;
		else if( GetArgStr( arg.fma, "-fma=", argv[i] ) )
			;
		else if( GetArgStr( arg.fmb, "-fmb=", argv[i] ) )
			;
		else if( IsArg( "-tr", argv[i] ) )
			arg.Transpose = true;
		else if( IsArg( "-ws", argv[i] ) )
			arg.WithinSection = true;
		else if( IsArg( "-nf", argv[i] ) )
			arg.NoFolds = true;
		else if( IsArg( "-sf", argv[i] ) )
			arg.SingleFold = true;
		else if( IsArg( "-v", argv[i] ) )
			arg.Verbose = true;
		else if( IsArg( "-heatmap", argv[i] ) )
			arg.Heatmap = true;
		else if( IsArg( "-dbgcor", argv[i] ) )
			dbgCor = true;
		else if( GetArgList( vD, "-Tmsh=", argv[i] ) ) {

			if( 6 == vD.size() )
				Tmsh.push_back( TForm( &vD[0] ) );
			else {
				printf(
				"main: WARNING: Bad format in -Tmsh [%s].\n",
				argv[i] );
			}
		}
		else if( GetArgList( vD, "-XYexp=", argv[i] ) ) {

			if( 2 == vD.size() )
				XYexp.push_back( Point( vD[0], vD[1] ) );
			else {
				printf(
				"main: WARNING: Bad format in -XYexp [%s].\n",
				argv[i] );
			}
		}
		else {
			printf( "Did not understand option '%s'.\n", argv[i] );
			return false;
		}
	}

// Decode labels in key

	if( !key ||
		(4 != sscanf( key, "%d/%d@%d/%d",
				&A.layer, &A.tile,
				&B.layer, &B.tile )) ) {

		printf( "main: Usage: ptest <za/ia@zb/ib>.\n" );
		return false;
	}

// Rename stdout using image labels

	OpenPairLog( A.layer, A.tile, B.layer, B.tile );

	printf( "\n---- dmesh start ----\n" );

// Record start time

	time_t	t0 = time( NULL );
	printf( "main: Start: %s\n", ctime(&t0) );

// Get default parameters

	if( !ReadMatchParams( mch, A.layer, B.layer ) )
		return false;

// Which file params to use according to (same,cross) layer

	double	cSCALE=1, cXSCALE=1, cYSCALE=1, cSKEW=0;
	int		cDfmFromTab;

	if( A.layer == B.layer ) {

		cDfmFromTab	= mch.TAB2DFM_SL;

		//ctx.Tdfm = identity (default)
		ctx.XYCONF	= mch.XYCONF_SL;
		ctx.NBMXHT	= mch.NBMXHT_SL;
		ctx.HFANGDN	= mch.HFANGDN_SL;
		ctx.HFANGPR	= mch.HFANGPR_SL;
		ctx.RTRSH	= mch.RTRSH_SL;
		ctx.RIT		= mch.RIT_SL;
		ctx.RFA		= mch.RFA_SL;
		ctx.RFT		= mch.RFT_SL;
		ctx.OLAP2D	= mch.OLAP2D_SL;
		ctx.MODE	= mch.MODE_SL;
		ctx.THMDEC	= mch.THMDEC_SL;
		ctx.OLAP1D	= mch.OLAP1D_SL;
		ctx.LIMXY	= mch.LIMXY_SL;
		ctx.OPT		= mch.OPT_SL;
	}
	else {

		cSCALE	= mch.SCALE;
		cXSCALE	= mch.XSCALE;
		cYSCALE	= mch.YSCALE;
		cSKEW	= mch.SKEW;

		ctx.Tdfm.ComposeDfm( cSCALE, cXSCALE, cYSCALE, 0, cSKEW );

		cDfmFromTab	= mch.TAB2DFM_XL;

		ctx.XYCONF	= mch.XYCONF_XL;
		ctx.NBMXHT	= mch.NBMXHT_XL;
		ctx.HFANGDN	= mch.HFANGDN_XL;
		ctx.HFANGPR	= mch.HFANGPR_XL;
		ctx.RTRSH	= mch.RTRSH_XL;
		ctx.RIT		= mch.RIT_XL;
		ctx.RFA		= mch.RFA_XL;
		ctx.RFT		= mch.RFT_XL;
		ctx.OLAP2D	= mch.OLAP2D_XL;
		ctx.MODE	= mch.MODE_XL;
		ctx.THMDEC	= mch.THMDEC_XL;
		ctx.OLAP1D	= mch.OLAP1D_XL;
		ctx.LIMXY	= mch.LIMXY_XL;
		ctx.OPT		= true;
	}

// Fetch Til2Img entries

	printf( "\n---- Input images ----\n" );

	IDBReadImgParams( idb );

	if( !IDBTil2Img( A.t2i, idb, A.layer, A.tile, _arg.ima ) ||
		!IDBTil2Img( B.t2i, idb, B.layer, B.tile, _arg.imb ) ) {

		return false;
	}

	PrintTil2Img( stdout, 'A', A.t2i );
	PrintTil2Img( stdout, 'B', B.t2i );

	printf( "\n" );

// Starting TForm A -> B

	if( _arg.Tab.size() )
		Tab = _arg.Tab[0];
	else
		AToBTrans( Tab, A.t2i.T, B.t2i.T );

// Extract file name as useful label

	A.file = FileCloneNamePart( A.t2i.path.c_str() );
	B.file = FileCloneNamePart( B.t2i.path.c_str() );

// Commandline overrides

	printf( "\n---- Command-line overrides ----\n" );

	if( _arg.Tab.size() ) {

		printf( "Tab=" );
		_arg.Tab[0].PrintTransform();
	}

	int	altTdfm = false;

	if( _arg.Tdfm.size() ) {

		ctx.Tdfm	= _arg.Tdfm[0];
		altTdfm		= true;
	}
	else {

		if( _arg.SCALE != 999.0 ) {
			cSCALE	= _arg.SCALE;
			altTdfm	= true;
			printf( "SCALE=%g\n", _arg.SCALE );
		}

		if( _arg.XSCALE != 999.0 ) {
			cXSCALE	= _arg.XSCALE;
			altTdfm	= true;
			printf( "XSCALE=%g\n", _arg.XSCALE );
		}

		if( _arg.YSCALE != 999.0 ) {
			cYSCALE	= _arg.YSCALE;
			altTdfm	= true;
			printf( "YSCALE=%g\n", _arg.YSCALE );
		}

		if( _arg.SKEW != 999.0 ) {
			cSKEW	= _arg.SKEW;
			altTdfm	= true;
			printf( "SKEW=%g\n", _arg.SKEW );
		}

		if( altTdfm )
			ctx.Tdfm.ComposeDfm( cSCALE, cXSCALE, cYSCALE, 0, cSKEW );
	}

	if( !altTdfm && cDfmFromTab ) {

		TForm	R;
		R.NUSetRot( -RadiansFromAffine( Tab ) );

		ctx.Tdfm = Tab;
		ctx.Tdfm.SetXY( 0, 0 );
		MultiplyTrans( ctx.Tdfm, R, TForm( ctx.Tdfm ) );
	}

	printf( "Tdfm=" );
	ctx.Tdfm.PrintTransform();

	if( _arg.MODE ) {
		ctx.MODE = _arg.MODE;
		printf( "MODE=%c\n", _arg.MODE );
	}

	if( ctx.MODE == 'Z' ) {
		ctx.MODE = 'C';
		arg.CTR = 0.0;
		printf( "MODE=C (was Z)\n", arg.CTR );
	}
	else if( ctx.MODE == 'M' ) {
		ctx.MODE = 'N';
		arg.CTR = 0.0;
		printf( "MODE=N (was M)\n", arg.CTR );
	}

	if( arg.CTR != 999.0 )
		printf( "CTR=%g\n", arg.CTR );

	printf( "\n" );

	return true;
}


