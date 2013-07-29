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
			_dummy_change_for( dummy_map, idx, which );
		times( &tnow );
		_dummy_change_for( dummy_map, idx, BIT_UNSET );
		
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

#define DISCOVER_NAME_LENGTH ( sizeof( "discover_X_XXXXXXXXXXX.dat" ) )
void bench_discover( map_t *dummy_map,
	map_t *hitmap,
	size_t end_idx,
	enum hitmap_mark which
) {
	struct tms tprev;
	struct tms tnow;

	char fname[ DISCOVER_NAME_LENGTH ];
	snprintf( fname,
		DISCOVER_NAME_LENGTH,
		"discover_%d_%lu.dat",
		which,
		( long unsigned int ) end_idx
	);
	FILE *discover_graph = fopen( fname, "w+" );

	for( size_t idx = 0;
		idx < end_idx;
		++idx
	) {
		size_t iter_num = 10000000ul / end_idx * 600ul ;
		fprintf( discover_graph,
			"%lu\t",
			( long unsigned int ) idx
		);
		_dummy_change_for( dummy_map, idx, which );
		
		times( &tprev );
		for( size_t cyc = 0; cyc < iter_num; ++cyc )
			assert( _dummy_discover( dummy_map, 0, which ) == idx );
		times( &tnow );

		_dummy_change_for( dummy_map,
			idx,
			( which == BIT_SET ) ? BIT_UNSET : BIT_SET
		);
		fprintf( discover_graph,
			"%lu\t",
			( tnow.tms_utime - tprev.tms_utime )
		);
		
		hitmap_change_for( hitmap, idx, which );
		
		times( &tprev );
		for( size_t cyc = 0; cyc < iter_num; ++cyc )
			assert( _hitmap_discover( hitmap, 0, which ) == idx );
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

void *thread_bench_discover( void *idx ) {
	map_t *dummy_map;
	map_t *hitmap;
	dummy_map = malloc( _dummy_calc_sz( 3 * _WORD_POW + 3 ) );
	hitmap = malloc(
		hitmap_calc_sz( 3 * _WORD_POW + 3 )
	);
	hitmap_init( hitmap, 3 * _WORD_POW + 3 );
	_dummy_init( dummy_map, 3 * _WORD_POW + 3 );

	bench_discover( dummy_map, hitmap, ( size_t ) idx, BIT_SET );

	free( dummy_map );
	free( hitmap );
}

int main( void ) {
	pthread_t bench_600, bench_1200, bench_10000;
	pthread_create( &bench_600, NULL, thread_bench_discover, (void *) 600ul );
	pthread_create( &bench_1200, NULL, thread_bench_discover, (void *) 1200ul );
	pthread_create( &bench_10000,
		NULL,
		thread_bench_discover,
		(void *) 10000ul
	);
	pthread_join( bench_600, NULL );
	pthread_join( bench_1200, NULL );
	pthread_join( bench_10000, NULL );
	
	return 0;
}
