

#include	"File.h"

#include	<ctype.h>
#include	<errno.h>
#include	<string.h>






/* --------------------------------------------------------------- */
/* FileOpenOrDie ------------------------------------------------- */
/* --------------------------------------------------------------- */

FILE *FileOpenOrDie(
	const char	*name,
	const char	*rw,
	FILE		*flog )
{
	FILE	*f = fopen( name, rw );

	if( !f ) {

		if( !flog )
			flog = stdout;

		fprintf( flog,
		"Can't open file [%s] for op [%s] errno [%d].\n",
		name, rw, errno );

		exit( 42 );
	}

	return f;
}

/* --------------------------------------------------------------- */
/* FileScriptPerms ----------------------------------------------- */
/* --------------------------------------------------------------- */

void FileScriptPerms( const char *path )
{
	char	buf[2048];

	sprintf( buf, "chmod ug=rwx,o=rx %s", path );
	system( buf );
}

/* --------------------------------------------------------------- */
/* FileNamePtr --------------------------------------------------- */
/* --------------------------------------------------------------- */

const char* FileNamePtr( const char *path )
{
	const char	*name;

	if( name = strrchr( path, '/' ) )
		++name;
	else
		name = path;

	return name;
}

/* --------------------------------------------------------------- */
/* FileDotPtr ---------------------------------------------------- */
/* --------------------------------------------------------------- */

const char* FileDotPtr( const char *path )
{
	const char	*dot;

	if( !(dot = strrchr( path, '.' )) )
		dot = path + strlen( path );

	return dot;
}

/* --------------------------------------------------------------- */
/* FileCloneNamePart --------------------------------------------- */
/* --------------------------------------------------------------- */

char* FileCloneNamePart( const char *path )
{
	char		buf[2048];
	const char	*name	= FileNamePtr( path ),
				*dot	= FileDotPtr( name );

	sprintf( buf, "%.*s", int(dot - name), name );

	return strdup( buf );
}

/* --------------------------------------------------------------- */
/* FileExtIsXML -------------------------------------------------- */
/* --------------------------------------------------------------- */

bool FileExtIsXML( const char *path )
{
	bool		isxml	= false;
	const char	*dot	= strrchr( path, '.' );

	if( dot
		&& strlen( path ) - (++dot - path) == 3
		&& toupper( *dot++ ) == 'X'
		&& toupper( *dot++ ) == 'M'
		&& toupper( *dot++ ) == 'L' ) {

		isxml = true;
	}

	return isxml;
}


