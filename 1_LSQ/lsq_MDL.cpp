

#include	"lsq_MDL.h"
#include	"lsq_Types.h"

#include	"GenDefs.h"
#include	"File.h"

#include	<math.h>


/* --------------------------------------------------------------- */
/* PrintMagnitude ------------------------------------------------ */
/* --------------------------------------------------------------- */

void MDL::PrintMagnitude( const vector<double> &X )
{
	int	k = X.size() - NX;

	if( k >= 0 ) {

		double	mag	= sqrt( X[k]*X[k] + X[k+1]*X[k+1] );

		printf( "Final magnitude is %g\n", mag );
	}

	fflush( stdout );
}

/* --------------------------------------------------------------- */
/* NewAffine ----------------------------------------------------- */
/* --------------------------------------------------------------- */

#if 0
void MDL::NewAffine(
	vector<double>	&X,
	vector<LHSCol>	&LHS,
	vector<double>	&RHS,
	double			sc,
	double			same_strength,
	double			square_strength,
	int				nTr,
	int				itr )
{
	SetPointPairs( LHS, RHS, sc, same_strength );
	SetIdentityTForm( LHS, RHS, itr );

// ------------------------------------------------------------
// Set matrix elements to translation values for subset of tiles
// ------------------------------------------------------------

#if 0
// set translations (scaled down)

	vector<double>	T;
	double			wt = 1;

	SolveXYOnly( T, nTr );

	int	nr = vRgn.size();

	for( int ir = 0; ir < nr; ++ir ) {

		const RGN&	R = vRgn[ir];
		int			i = R.itr;

		if( i < 0 )
			continue;

		int	col = R.id / 1000 - R.z * 1000;
		int	row = R.id - (R.z * 1000 + col) * 1000;

		if( col == 8 || col == 75 || row == 10 || row == 154 )
			;
		else
			continue;

		int	j;

		j = i * NX + 2;
		AddConstraint( LHS, RHS, 1, &j, &wt, T[i*2]*wt/sc );

		j = i * NX + 5;
		AddConstraint( LHS, RHS, 1, &j, &wt, T[i*2+1]*wt/sc );
	}
#endif

// ------------------------------------------------------------
// Just set matrix elements to translation values
// ------------------------------------------------------------

#if 1
// set translations (scaled down)

	vector<double>	T;
	double			wt	= 0.01;	// 0.01

	SolveXYOnly( T, nTr );

	for( int i = 0; i < nTr; ++i ) {

		int	j;

		j = i * NX + 2;
		AddConstraint( LHS, RHS, 1, &j, &wt, T[i*2]*wt/sc );

		j = i * NX + 5;
		AddConstraint( LHS, RHS, 1, &j, &wt, T[i*2+1]*wt/sc );
	}
#endif

// ------------------------------------------------------------
// Alternatively, set Aff(p) = Trans(p)
// ------------------------------------------------------------

#if 0
// set translations (scaled down)

	vector<double>	T;
	double			wt = 0.1;
	int				nc	= vAllC.size();

	SolveXYOnly( T, nTr );

	for( int i = 0; i < nc; ++i ) {

		const Constraint &C = vAllC[i];

		if( !C.used || !C.inlier )
			continue;

		double	x1 = C.p1.x * wt / sc,
				y1 = C.p1.y * wt / sc,
				x2 = C.p2.x * wt / sc,
				y2 = C.p2.y * wt / sc;
		int		j  = vRgn[C.r1].itr,
				k  = vRgn[C.r2].itr,
				J  = j,
				K  = k;

		j *= NX;
		k *= NX;
		J *= 2;
		K *= 2;

		// A(p) = T(p)

		double	v1[3] = { x1,  y1,  wt};
		int		j1[3] = {  j, j+1, j+2},
				j2[3] = {j+3, j+4, j+5};

		AddConstraint( LHS, RHS, 3, j1, v1, T[J  ]*wt/sc + x1 );
		AddConstraint( LHS, RHS, 3, j2, v1, T[J+1]*wt/sc + y1 );

		double	v2[3] = { x2,  y2,  wt};
		int		k1[3] = {  k, k+1, k+2},
				k2[3] = {k+3, k+4, k+5};

		AddConstraint( LHS, RHS, 3, k1, v2, T[K  ]*wt/sc + x2 );
		AddConstraint( LHS, RHS, 3, k2, v2, T[K+1]*wt/sc + y2 );
	}
#endif

// solve

	//SolveWithSquareness( X, LHS, RHS, nTr, square_strength );

	printf( "Solve with [fixed translation].\n" );
	WriteSolveRead( X, LHS, RHS, false );
	PrintMagnitude( X );

	RescaleAll( X, sc );
}
#endif

/* --------------------------------------------------------------- */
/* Bounds -------------------------------------------------------- */
/* --------------------------------------------------------------- */

void MDL::Bounds(
	double					&xbnd,
	double					&ybnd,
	vector<double>			&X,
	int						gW,
	int						gH,
	const vector<double>	&lrbt,
	double					degcw,
	FILE					*FOUT )
{
	printf( "---- Global bounds ----\n" );

// Transform each included regions's rectangle to global
// space, including any global rotation (degcw) and find
// bounds over whole set.

	double	xmin, xmax, ymin, ymax;
	int		nr = vRgn.size();

	if( lrbt.size() ) {

		xmin = lrbt[0];
		xmax = lrbt[1];
		ymin = lrbt[2];
		ymax = lrbt[3];
	}
	else {

		xmin =  BIGD;
		xmax = -BIGD;
		ymin =  BIGD;
		ymax = -BIGD;

		if( degcw )
			RotateAll( X, degcw );

		for( int i = 0; i < nr; ++i ) {

			int	itr = vRgn[i].itr;

			if( itr < 0 )
				continue;

			vector<Point>	cnr( 4 );

			cnr[0] = Point(  0.0, 0.0 );
			cnr[1] = Point( gW-1, 0.0 );
			cnr[2] = Point( gW-1, gH-1 );
			cnr[3] = Point(  0.0, gH-1 );

			L2GPoint( cnr, X, itr );

			for( int k = 0; k < 4; ++k ) {

				xmin = fmin( xmin, cnr[k].x );
				xmax = fmax( xmax, cnr[k].x );
				ymin = fmin( ymin, cnr[k].y );
				ymax = fmax( ymax, cnr[k].y );
			}
		}
	}

	printf( "Propagate bounds with option -lrbt=%f,%f,%f,%f\n\n",
	xmin, xmax, ymin, ymax );

// Translate all transforms to put global origin at ~(0,0).

	NewOriginAll( X, xmin, ymin );

	xmax = ceil( xmax - xmin + 1 );
	ymax = ceil( ymax - ymin + 1 );
	xmin = 0;
	ymin = 0;

// Open GNUPLOT files for debugging

	FILE	*fEven		= FileOpenOrDie( "pf.even", "w" ),
			*fOdd		= FileOpenOrDie( "pf.odd", "w" ),
			*fLabEven	= FileOpenOrDie( "pf.labels.even", "w" ),
			*fLabOdd	= FileOpenOrDie( "pf.labels.odd", "w" );

// Write rects and labels

	for( int i = 0; i < nr; ++i ) {

		int	itr = vRgn[i].itr;

		if( itr < 0 )
			continue;

		vector<Point>	cnr( 4 );
		double			xmid = 0.0, ymid = 0.0;

		cnr[0] = Point(  0.0, 0.0 );
		cnr[1] = Point( gW-1, 0.0 );
		cnr[2] = Point( gW-1, gH-1 );
		cnr[3] = Point(  0.0, gH-1 );

		L2GPoint( cnr, X, itr );

		for( int k = 0; k < 4; ++k ) {
			xmid += cnr[k].x;
			ymid += cnr[k].y;
		}

		xmid /= 4.0;
		ymid /= 4.0;

		// select even or odd reporting

		FILE	*f, *flab;
		int		color;

		if( vRgn[i].z & 1 ) {
			f		= fOdd;
			flab	= fLabOdd;
			color	= 1;
		}
		else {
			f		= fEven;
			flab	= fLabEven;
			color	= 2;
		}

		// transformed rect

		for( int k = 0; k < 5; ++k )
			fprintf( f, "%f %f\n", cnr[k%4].x, cnr[k%4].y );

		fprintf( f, "\n" );

		// label

		fprintf( flab, "set label \"%d:%d.%d \" at %f,%f tc lt %d\n",
		vRgn[i].z, vRgn[i].id, vRgn[i].rgn, xmid, ymid, color );
	}

// Close files

	fclose( fLabOdd );
	fclose( fLabEven );
	fclose( fOdd );
	fclose( fEven );

// Report

	fprintf( FOUT, "BBOX %f %f %f %f\n", xmin, ymin, xmax, ymax );

	printf( "Bounds of global image are x=[%f %f] y=[%f %f].\n\n",
	xmin, xmax, ymin, ymax );

	xbnd = xmax;
	ybnd = ymax;
}


