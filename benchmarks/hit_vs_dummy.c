#include <hitmap.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <atomic_ops.h>

#include <valgrind/callgrind.h>

void bench_change( map_t *dummy_map,
	map_t *hitmap,
	enum hitmap_mark which
) {
	for( size_t idx = 0;
		idx < ( 1ul << ( 3 * _WORD_POW + 3 ) );
		++idx
	) {
		for( int cyc = 0; cyc < 1000; ++cyc )
			_dummy_change_for( dummy_map, idx, which );
		_dummy_change_for( dummy_map, idx, BIT_UNSET );

		for( int cyc = 0; cyc < 1000; ++cyc )
			hitmap_change_for( hitmap, idx, which );
		hitmap_change_for( hitmap, idx, BIT_UNSET );
	}
}

int main( void ) {
	map_t *dummy_map;
	map_t *hitmap;
	dummy_map = malloc( sizeof( map_t ) +
		sizeof( AO_t ) * ( 1ul << ( 2 * _WORD_POW + 3 ) )
	);
	hitmap = malloc(
		hitmap_calc_sz( 3 * _WORD_POW + 3 )
	);

	dummy_map->len_pow = 3 * _WORD_POW + 3;
	dummy_map->bits_border = NULL;
	memset( dummy_map->bits,
		0,
		sizeof( AO_t ) * ( 1ul << ( 2 * _WORD_POW + 3 ) )
	);
	
	hitmap_init( hitmap, 3 * _WORD_POW + 3 );

	CALLGRIND_START_INSTRUMENTATION;
	char id[ 10 ];
	for( size_t idx = 0;
		idx < 6000;
		++idx
	) {
		CALLGRIND_ZERO_STATS;

		_dummy_change_for( dummy_map, idx, BIT_SET );
		assert( _dummy_discover( dummy_map, 0, BIT_SET ) == idx );
		_dummy_change_for( dummy_map, idx, BIT_UNSET );

		
		hitmap_change_for( hitmap, idx, BIT_SET );
		assert( _hitmap_discover( hitmap, 0, BIT_SET ) == idx );
		hitmap_change_for( hitmap, idx, BIT_UNSET );

		snprintf( id, 9, "%lu", ( long unsigned int ) idx );
		CALLGRIND_DUMP_STATS_AT( id );
	}
	CALLGRIND_STOP_INSTRUMENTATION;

	free( dummy_map );
	free( hitmap );

	return 0;
}
