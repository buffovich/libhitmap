#include <hitmap.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/times.h>
#include <atomic_ops.h>

#define CHANGE_NAME_LENGTH ( sizeof( "change_.dat" ) + 2 )
void bench_change( map_t *dummy_map,
	map_t *hitmap,
	enum hitmap_mark which
) {
	struct tms tprev;
	struct tms tnow;

	char fname[ CHANGE_NAME_LENGTH ];
	snprintf( fname, CHANGE_NAME_LENGTH, "change_%d.dat", which );
	FILE *change_graph = fopen( fname, "w+" );

	for( size_t idx = 0;
		idx < ( 1ul << ( 3 * _WORD_POW + 3 ) );
		++idx
	) {
		fprintf( change_graph,
			"%lu\t",
			( long unsigned int ) idx
		);
		
		times( &tprev );
		for( int cyc = 0; cyc < 1000; ++cyc )
			dummy_change_for( dummy_map, idx, which );
		times( &tnow );
		dummy_change_for( dummy_map, idx, BIT_UNSET );
		
		fprintf( change_graph,
			"%lu\t",
			( tnow.tms_utime - tprev.tms_utime )
		);
		
		times( &tprev );
		for( int cyc = 0; cyc < 1000; ++cyc )
			hitmap_change_for( hitmap, idx, which );
		times( &tnow );
		hitmap_change_for( hitmap, idx, BIT_UNSET );
		
		fprintf( change_graph,
			"%lu\n",
			( tnow.tms_utime - tprev.tms_utime )
		);
	}
}

#define DISCOVER_NAME_LENGTH ( sizeof( "discover_.dat" ) + 2 )
void bench_discover( map_t *dummy_map,
	map_t *hitmap,
	enum hitmap_mark which
) {
	struct tms tprev;
	struct tms tnow;

	char fname[ DISCOVER_NAME_LENGTH ];
	snprintf( fname, DISCOVER_NAME_LENGTH, "discover_%d.dat", which );
	FILE *discover_graph = fopen( fname, "w+" );

	for( size_t idx = 0;
		idx < 642;
		++idx
	) {
		fprintf( discover_graph,
			"%lu\t",
			idx
		);
		dummy_change_for( dummy_map, idx, which );
		
		times( &tprev );
		for( int cyc = 0; cyc < 10000000; ++cyc )
			assert( dummy_discover( dummy_map, 0, which ) == idx );
		times( &tnow );

		dummy_change_for( dummy_map,
			idx,
			( which == BIT_SET ) ? BIT_UNSET : BIT_SET
		);
		fprintf( discover_graph,
			"%lu\t",
			( tnow.tms_utime - tprev.tms_utime )
		);
		
		hitmap_change_for( hitmap, idx, which );
		
		times( &tprev );
		for( int cyc = 0; cyc < 10000000; ++cyc )
			assert( hitmap_discover( hitmap, 0, which ) == idx );
		times( &tnow );

		hitmap_change_for( hitmap, idx,
			( which == BIT_SET ) ? BIT_UNSET : BIT_SET
		);
		fprintf( discover_graph,
			"%lu\n",
			( tnow.tms_utime - tprev.tms_utime )
		);

		fflush( discover_graph );
	}

	fclose( discover_graph );
}

int main( void ) {
	map_t *dummy_map;
	map_t *hitmap;
	dummy_map = malloc( dummy_calc_sz( 3 * _WORD_POW + 3 ) );
	hitmap = malloc(
		hitmap_calc_sz( 3 * _WORD_POW + 3 )
	);
	hitmap_init( hitmap, 3 * _WORD_POW + 3 );
	dummy_init( dummy_map, 3 * _WORD_POW + 3 );

	//bench_change( dummy_map, hitmap, BIT_SET );
	//bench_change( dummy_map, hitmap, BIT_UNSET );
	bench_discover( dummy_map, hitmap, BIT_SET );
	//bench_discover( dummy_map, hitmap, BIT_UNSET );

	free( dummy_map );
	free( hitmap );
	
	return 0;
}