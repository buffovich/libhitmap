#define _GNU_SOURCE

#include <check.h>
#include <hitmap.h>

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/times.h>

START_TEST( test_calc_sz_normal )
{
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW ),
		sizeof( map_t ) + sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + 1 ),
		sizeof( map_t ) + 2 * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + 2 ),
		sizeof( map_t ) + 6 * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + 3 ),
		sizeof( map_t ) + 10 * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW - 1 ),
		sizeof( map_t ) + ( ( 1 << ( _WORD_POW - 1 ) ) + 2 ) * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW ),
		sizeof( map_t ) + ( ( 1 << _WORD_POW ) + 2 ) * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW + 1 ),
		sizeof( map_t ) + ( ( 1 << ( _WORD_POW + 1 ) ) + 4 ) * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW + 2 ),
		sizeof( map_t ) +
			( ( 1 << ( _WORD_POW + 2 ) ) + 8 + 2 ) * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW + _WORD_POW - 1 ),
		sizeof( map_t ) +
			( ( 1 << ( _WORD_POW + _WORD_POW - 1 ) ) +
				( 1 << _WORD_POW ) + 2
			) * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW + _WORD_POW ),
		sizeof( map_t ) +
			( ( 1 << ( _WORD_POW + _WORD_POW ) ) +
				( 1 << ( _WORD_POW + 1 ) ) + 2
			) * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW + _WORD_POW + 1 ),
		sizeof( map_t ) +
			( ( 1 << ( _WORD_POW + _WORD_POW + 1 ) ) +
				( 1 << ( _WORD_POW + 2 ) ) + 4
			) * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + _WORD_POW + _WORD_POW + 2 ),
		sizeof( map_t ) +
			( ( 1 << ( _WORD_POW + _WORD_POW + 2 ) ) +
				( 1 << ( _WORD_POW + 3 ) ) + 8 + 2
			) * sizeof( AO_t )
	);
}
END_TEST

START_TEST( test_init_normal )
{
	size_t sz = hitmap_calc_sz( 3 * _WORD_POW );
	map_t *map = malloc( sz + 2 );

	for( size_t cyc = 0; cyc < sz; ++cyc )
		( ( char* ) map )[ cyc ] = 3;

	( ( char* ) map )[ sz ] = 111;
	( ( char* ) map )[ sz + 1 ] = 112;
	hitmap_init( map, 3 * _WORD_POW );

	ck_assert_int_eq( ( ( char* ) map )[ sz ], 111 );
	ck_assert_int_eq( ( ( char* ) map )[ sz + 1 ], 112 );

	for( size_t cyc = 0; cyc < ( 1 << ( 2 * _WORD_POW ) ); ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	int sw = 0;
	for( size_t cyc = ( 1 << ( 2 * _WORD_POW ) );
		cyc < ( 1 << ( 2 * _WORD_POW ) ) + ( 1 << _WORD_POW ) * 2 + 2;
		++cyc, sw = !sw
	)
		ck_assert_int_eq( map->bits[ cyc ], sw ? SIZE_MAX : 0 );

	map->bits[
		( 1 << ( 2 * _WORD_POW - 1 ) ) + ( 1 << ( _WORD_POW - 1 ) ) * 2 + 2
	] = 111;
	map->bits[
		( 1 << ( 2 * _WORD_POW - 1 ) ) + ( 1 << ( _WORD_POW - 1 ) ) * 2 + 3
	] = 112;
	hitmap_init( map, 3 * _WORD_POW - 1 );
	ck_assert_int_eq(
		map->bits[
			( 1 << ( 2 * _WORD_POW - 1 ) ) + ( 1 << ( _WORD_POW - 1 ) ) * 2 + 2
		],
		111
	);
	ck_assert_int_eq(
		map->bits[
			( 1 << ( 2 * _WORD_POW - 1 ) ) + ( 1 << ( _WORD_POW - 1 ) ) * 2 + 3
		],
		112
	);

	for( size_t cyc = 0; cyc < ( 1 << ( 2 * _WORD_POW - 1 ) ); ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	sw = 0;
	for( size_t cyc = ( 1 << ( 2 * _WORD_POW - 1 ) );
		cyc < ( 1 << ( 2 * _WORD_POW - 1 ) ) +
			( 1 << ( _WORD_POW - 1 ) ) * 2 + 2;
		++cyc, sw = !sw
	)
		ck_assert_int_eq( map->bits[ cyc ], sw ? SIZE_MAX : 0 );

	map->bits[
		( 1 << ( _WORD_POW + 1 ) ) + ( 1 << 1 ) * 2 + 1
	] = 111;
	map->bits[
		( 1 << ( _WORD_POW + 1 ) ) + ( 1 << 1 ) * 2 + 2
	] = 112;
	hitmap_init( map, 2 * _WORD_POW + 1 );
	ck_assert_int_eq(
		map->bits[
			( 1 << ( _WORD_POW + 1 ) ) + ( 1 << 1 ) * 2 + 1
		],
		111
	);
	ck_assert_int_eq(
		map->bits[
			( 1 << ( _WORD_POW + 1 ) ) + ( 1 << 1 ) * 2 + 2
		],
		112
	);

	for( size_t cyc = 0; cyc < ( 1 << ( _WORD_POW + 1 ) ); ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	sw = 0;
	for( size_t cyc = ( 1 << ( _WORD_POW + 1 ) );
		cyc < ( 1 << ( _WORD_POW + 1 ) ) +
			( 1 << 1 ) * 2;
		++cyc, sw = !sw
	)
		ck_assert_int_eq( map->bits[ cyc ], sw ? SIZE_MAX : 0 );

	free( map );
}
END_TEST

START_TEST( test_change_for_normal )
{
	size_t sz = hitmap_calc_sz( 3 * _WORD_POW );
	map_t *map = malloc( sz + 2 );
	( ( char* ) map )[ sz ] = 111;
	( ( char* ) map )[ sz + 1 ] = 112;
	hitmap_init( map, 3 * _WORD_POW );

	hitmap_change_for( map, 0, BIT_SET );
	hitmap_change_for( map, 2, BIT_SET );
	hitmap_change_for( map, 4, BIT_SET );

	ck_assert_int_eq( map->bits[ 0 ], 0x15 );
	size_t start_with = ( 1ul << ( 2 * _WORD_POW ) );
	for( size_t cyc = 1; cyc < start_with; ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	ck_assert_int_eq( map->bits[ start_with ], 1 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX );
	int neg = 0;
	for( size_t cyc = start_with + 2;
		cyc < start_with + ( 1ul << _WORD_POW ) * 2;
		++cyc, neg = ! neg
	)
		ck_assert_int_eq( map->bits[ cyc ], neg ? SIZE_MAX : 0 );

	start_with += ( 1ul << _WORD_POW ) * 2;
	ck_assert_int_eq( map->bits[ start_with ], 1 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX );

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc )
		hitmap_change_for( map, cyc, BIT_SET );

	ck_assert_int_eq( map->bits[ 0 ], SIZE_MAX );

	start_with = ( 1ul << ( 2 * _WORD_POW ) );
	for( size_t cyc = 1; cyc < start_with; ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	ck_assert_int_eq( map->bits[ start_with ], 1 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX ^ 1ul );

	neg = 0;
	for( size_t cyc = start_with + 2;
		cyc < start_with + ( 1ul << _WORD_POW ) * 2;
		++cyc, neg = ! neg
	)
		ck_assert_int_eq( map->bits[ cyc ], neg ? SIZE_MAX : 0 );

	start_with += ( 1ul << _WORD_POW ) * 2;
	ck_assert_int_eq( map->bits[ start_with ], 1 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, 1ul << _WORD_POW, BIT_SET );

	ck_assert_int_eq( map->bits[ 0 ], SIZE_MAX );
	ck_assert_int_eq( map->bits[ 1 ], 1 );

	start_with = ( 1ul << ( 2 * _WORD_POW ) );
	for( size_t cyc = 2; cyc < start_with; ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	ck_assert_int_eq( map->bits[ start_with ], 3 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX ^ 1ul );

	neg = 0;
	for( size_t cyc = start_with + 2;
		cyc < start_with + ( 1ul << _WORD_POW ) * 2;
		++cyc, neg = ! neg
	)
		ck_assert_int_eq( map->bits[ cyc ], neg ? SIZE_MAX : 0 );

	start_with += ( 1ul << _WORD_POW ) * 2;
	ck_assert_int_eq( map->bits[ start_with ], 1 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map,
		( 1ul << ( 3 * _WORD_POW - 1 ) ) + ( 1ul << ( _WORD_POW - 1 ) ),
		BIT_SET
	);

	ck_assert_int_eq( map->bits[ 0 ], SIZE_MAX );
	ck_assert_int_eq( map->bits[ 1 ], 1 );
	for( size_t cyc = 2; cyc < ( 1ul << ( 2 * _WORD_POW - 1 ) ); ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );
	ck_assert_int_eq(
		map->bits[ 1ul << ( 2 * _WORD_POW - 1 ) ],
		1ul << ( 1u << ( _WORD_POW - 1 ) )
	);
	start_with = ( 1ul << ( 2 * _WORD_POW ) );
	for( size_t cyc = ( 1ul << ( 2 * _WORD_POW - 1 ) ) + 1;
		cyc < start_with;
		++cyc
	)
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	ck_assert_int_eq( map->bits[ start_with ], 3 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX ^ 1ul );
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 ],
		1
	);
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 + 1 ],
		SIZE_MAX
	);

	start_with += ( 1ul << _WORD_POW ) * 2;
	ck_assert_int_eq(
		map->bits[ start_with ],
		1ul | ( 1ul << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX );

	for( size_t cyc = ( 1ul << ( 3 * _WORD_POW - 1 ) );
		cyc < (
			( 1ul << ( 3 * _WORD_POW - 1 ) ) +
			( 1ul << ( 2 * _WORD_POW ) )
		);
		++cyc
	)
		hitmap_change_for( map, cyc, BIT_SET );
	for( size_t cyc = 2; cyc < ( 1ul << ( 2 * _WORD_POW - 1 ) ); ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );
	for( size_t cyc = ( 1ul << ( 2 * _WORD_POW - 1 ) );
		cyc < (
			( 1ul << ( 2 * _WORD_POW - 1 ) ) +
			( 1ul << _WORD_POW )
		);
		++cyc
	)
		ck_assert_int_eq( map->bits[ cyc ], SIZE_MAX );

	for( size_t cyc = (
			( 1ul << ( 2 * _WORD_POW - 1 ) ) +
			( 1ul << _WORD_POW )
		);
		cyc < ( 1ul << ( 2 * _WORD_POW ) );
		++cyc
	)
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	start_with = 1ul << ( 2 * _WORD_POW );
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 ],
		SIZE_MAX
	);
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 + 1 ],
		0
	);
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 + 2 ],
		0
	);
	start_with += ( 1ul << _WORD_POW ) * 2;
	ck_assert_int_eq(
		map->bits[ start_with ],
		1ul | ( 1ul << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	ck_assert_int_eq( map->bits[ start_with + 1 ],
		SIZE_MAX ^ ( 1ul << ( 1ul << ( _WORD_POW - 1 ) ) )
	);

	hitmap_change_for( map,
		( 1ul << ( 3 * _WORD_POW - 1 ) ) + ( 1ul << ( _WORD_POW - 1 ) ),
		BIT_UNSET
	);
	ck_assert_int_eq( map->bits[ ( 1ul << ( 2 * _WORD_POW - 1 ) ) ],
		SIZE_MAX ^ ( 1ul << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	start_with = 1ul << ( 2 * _WORD_POW );
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 ],
		SIZE_MAX
	);
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 + 1 ],
		1
	);
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 1 ) ) * 2 + 2 ],
		0
	);
	start_with += ( 1ul << _WORD_POW ) * 2;
	ck_assert_int_eq(
		map->bits[ start_with ],
		1ul | ( 1ul << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	ck_assert_int_eq( map->bits[ start_with + 1 ],
		SIZE_MAX
	);

	for( size_t cyc = ( 1ul << ( 3 * _WORD_POW - 1 ) );
		cyc < (
			( 1ul << ( 3 * _WORD_POW - 1 ) ) +
			( 1ul << ( 2 * _WORD_POW ) )
		);
		++cyc
	)
		hitmap_change_for( map, cyc, BIT_UNSET );
	start_with = ( 1ul << ( 2 * _WORD_POW ) );
	for( size_t cyc = 2; cyc < start_with; ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	neg = 0;
	for( size_t cyc = start_with + 2;
		cyc < start_with + ( 1ul << _WORD_POW ) * 2;
		++cyc, neg = ! neg
	)
		ck_assert_int_eq( map->bits[ cyc ], neg ? SIZE_MAX : 0 );

	start_with += ( 1ul << _WORD_POW ) * 2;
	ck_assert_int_eq( map->bits[ start_with ], 1 );
	ck_assert_int_eq( map->bits[ start_with + 1 ], SIZE_MAX );

	ck_assert_int_eq( ( ( char* ) ( map->bits + start_with + 2 ) )[ 0 ], 111 );
	ck_assert_int_eq( ( ( char* ) ( map->bits + start_with + 2 ) )[ 1 ], 112 );

	sz = hitmap_calc_sz( 3 * _WORD_POW - 2 );
	( ( char* ) map )[ sz ] = 111;
	( ( char* ) map )[ sz + 1 ] = 112;
	hitmap_init( map, 3 * _WORD_POW - 2 );

	for( size_t cyc = ( 1ul << ( 3 * _WORD_POW - 3 ) );
		cyc < (
			( 1ul << ( 3 * _WORD_POW - 3 ) ) +
			( 1ul << ( 2 * _WORD_POW ) )
		);
		++cyc
	)
		hitmap_change_for( map, cyc, BIT_SET );
	for( size_t cyc = 2; cyc < ( 1ul << ( 2 * _WORD_POW - 3 ) ); ++cyc )
		ck_assert_int_eq( map->bits[ cyc ], 0 );
	for( size_t cyc = ( 1ul << ( 2 * _WORD_POW - 3 ) );
		cyc < (
			( 1ul << ( 2 * _WORD_POW - 3 ) ) +
			( 1ul << _WORD_POW )
		);
		++cyc
	)
		ck_assert_int_eq( map->bits[ cyc ], SIZE_MAX );

	for( size_t cyc = (
			( 1ul << ( 2 * _WORD_POW - 3 ) ) +
			( 1ul << _WORD_POW )
		);
		cyc < ( 1ul << ( 2 * _WORD_POW - 2 ) );
		++cyc
	)
		ck_assert_int_eq( map->bits[ cyc ], 0 );

	start_with = 1ul << ( 2 * _WORD_POW - 2 );
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 3 ) ) * 2 ],
		SIZE_MAX
	);
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 3 ) ) * 2 + 1 ],
		0
	);
	ck_assert_int_eq(
		map->bits[ start_with + ( 1ul << ( _WORD_POW - 3 ) ) * 2 + 2 ],
		0
	);
	start_with += ( 1ul << ( _WORD_POW - 2 ) ) * 2;
	ck_assert_int_eq(
		map->bits[ start_with ],
		0xF << ( 1ul << ( _WORD_POW - 1 ) )
	);
	ck_assert_int_eq( map->bits[ start_with + 1 ],
		SIZE_MAX ^ ( 0xF << ( 1ul << ( _WORD_POW - 1 ) ) )
	);

	ck_assert_int_eq( ( ( char* ) ( map->bits + start_with + 2 ) )[ 0 ], 111 );
	ck_assert_int_eq( ( ( char* ) ( map->bits + start_with + 2 ) )[ 1 ], 112 );

	free( map );
}
END_TEST

/*
START_TEST( test_has_normal )
{
	AO_t *map = malloc( hitmap_calc_sz( _WORD_POW + 3 ) * sizeof( AO_t ) );
	hitmap_init( map, _WORD_POW + 3 );

	ck_assert( ! hitmap_has( map, _WORD_POW + 3, BIT_SET ) );
	for( size_t idx = 0; idx < ( 1 << ( _WORD_POW + 3 ) ); ++idx ) {
		ck_assert( hitmap_has( map, _WORD_POW + 3, BIT_UNSET ) );
		hitmap_change_for( map, idx, BIT_SET );
		ck_assert( hitmap_has( map, _WORD_POW + 3, BIT_SET ) );
	}

	ck_assert( ! hitmap_has( map, _WORD_POW + 3, BIT_UNSET ) );

	free( map );
}
END_TEST
*/

void dummy_change_for( AO_t *map,
	int len_pow,
	size_t idx,
	enum hitmap_mark is_put
) {
	assert( map != NULL );
	assert( ( is_put == BIT_SET ) || ( is_put == BIT_UNSET ) );
	assert( idx < ( 1ul << len_pow ) );
	
	switch( is_put ) {
		case BIT_SET:
			map[ idx >> _WORD_POW ] |=
				1ul << ( idx & ( HITMAP_INITIAL_SIZE - 1 ) );
			break;
		case BIT_UNSET:
			map[ idx >> _WORD_POW ] &=
				~ ( 1ul << ( idx & ( HITMAP_INITIAL_SIZE - 1 ) ) );
			break;
	}
}

inline static size_t _ffcl( AO_t mword ) {
	return ffsl( ~ mword );
}

size_t dummy_discover( AO_t *map,
	int len_pow,
	size_t start_idx,
	enum hitmap_mark which
) {
	assert( map != NULL );
	assert( ( which == BIT_SET ) || ( which == BIT_UNSET ) );
	assert( start_idx < ( 1ul << len_pow ) );

	size_t wcapacity = 1ul << ( len_pow - _WORD_POW );
	size_t bitn = 0;
	switch( which ) {
		case BIT_SET:
			for( size_t cyc = start_idx >> _WORD_POW;
				cyc < wcapacity;
				++cyc
			) {
				bitn = ( size_t ) ffsl( map[ cyc ] );
				if( bitn != 0 )
					return ( cyc << _WORD_POW ) | ( bitn - 1 );
			}
			break;
		case BIT_UNSET:
			for( size_t cyc = start_idx >> _WORD_POW;
				cyc < wcapacity;
				++cyc
			) {
				bitn = _ffcl( map[ cyc ] );
				if( bitn != 0 )
					return ( cyc << _WORD_POW ) | ( bitn - 1 );
			}
			break;
	}
}

START_TEST( test_discover_normal )
{
	size_t sz = hitmap_calc_sz( 4 * _WORD_POW + 1 );
	map_t *map = malloc( sz );
	hitmap_init( map, 2 * _WORD_POW );

	ck_assert_int_eq(
		hitmap_discover( map, 0, BIT_SET ), SIZE_MAX
	);
	ck_assert_int_eq(
		hitmap_discover( map,
			( 1ul << ( 2 * _WORD_POW - 1 ) ) - 1,
			BIT_SET
		),
		SIZE_MAX
	);

	ck_assert_int_eq( hitmap_discover( map, 0, BIT_UNSET ), 0 );
	ck_assert_int_eq(
		hitmap_discover( map,
			( 1ul << ( 2 * _WORD_POW - 1 ) ) - 1,
			BIT_UNSET
		),
		( 1ul << ( 2 * _WORD_POW - 1 ) ) - 1
	);

	for( size_t cyc = 0; cyc < ( 1ul << ( 2 * _WORD_POW ) ); ++cyc ) {
		hitmap_change_for( map, cyc, BIT_SET );
		ck_assert_int_eq(
			hitmap_discover( map, 0, BIT_SET ),
			cyc
		);
		hitmap_change_for( map, cyc, BIT_UNSET );
	}

	hitmap_change_for( map, 0, BIT_SET );
	hitmap_change_for( map, 1, BIT_SET );
	hitmap_change_for( map, 3, BIT_SET );
	hitmap_change_for( map, 7, BIT_SET );
	hitmap_change_for( map, 15, BIT_SET );
	hitmap_change_for( map, 31, BIT_SET );
	hitmap_change_for( map, 33, BIT_SET );
	hitmap_change_for( map, 63, BIT_SET );
	hitmap_change_for( map, 127, BIT_SET );
	hitmap_change_for( map, 1ul << ( 2 * _WORD_POW - 2 ), BIT_SET );
	hitmap_change_for( map, 1ul << ( 2 * _WORD_POW - 1 ), BIT_SET );
	hitmap_change_for( map, 
		( 1ul << ( 2 * _WORD_POW ) ) - 1,
		BIT_SET
	);

	ck_assert_int_eq( hitmap_discover( map, 0, BIT_SET ), 0 );
	ck_assert_int_eq( hitmap_discover( map, 1, BIT_SET ), 1 );
	ck_assert_int_eq( hitmap_discover( map, 2, BIT_SET ), 3 );
	ck_assert_int_eq( hitmap_discover( map, 5, BIT_SET ), 7 );
	ck_assert_int_eq( hitmap_discover( map, 8, BIT_SET ), 15 );
	ck_assert_int_eq( hitmap_discover( map, 19, BIT_SET ), 31 );
	ck_assert_int_eq( hitmap_discover( map, 32, BIT_SET ), 33 );
	ck_assert_int_eq( hitmap_discover( map, 34, BIT_SET ), 63 );
	ck_assert_int_eq( hitmap_discover( map, 66, BIT_SET ), 127 );
	ck_assert_int_eq(
		hitmap_discover( map, 150, BIT_SET ),
		1ul << ( 2 * _WORD_POW - 2 )
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << ( 2 * _WORD_POW - 2 ) ) + 1, BIT_SET ),
		1ul << ( 2 * _WORD_POW - 1 )
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << ( 2 * _WORD_POW - 1 ) ) + 1, BIT_SET ),
		( 1ul << ( 2 * _WORD_POW ) ) - 1
	);

	// test for 0 layer + 1 layer with 2 words
	hitmap_init( map, 2 * _WORD_POW + 1 );

	ck_assert_int_eq(
		hitmap_discover( map, 0, BIT_SET ), SIZE_MAX
	);
	ck_assert_int_eq(
		hitmap_discover( map,
			( 1ul << ( 2 * _WORD_POW ) ) - 1,
			BIT_SET
		),
		SIZE_MAX
	);

	ck_assert_int_eq( hitmap_discover( map, 0, BIT_UNSET ), 0 );
	ck_assert_int_eq(
		hitmap_discover( map,
			( 1ul << ( 2 * _WORD_POW ) ) - 1,
			BIT_UNSET
		),
		( 1ul << ( 2 * _WORD_POW ) ) - 1
	);

	for( size_t cyc = 0; cyc < ( 1ul << ( 2 * _WORD_POW + 1 ) ); ++cyc ) {
		hitmap_change_for( map, cyc, BIT_SET );
		ck_assert_int_eq(
			hitmap_discover( map, 0, BIT_SET ),
			cyc
		);
		hitmap_change_for( map, cyc, BIT_UNSET );
	}

	hitmap_change_for( map, 0, BIT_SET );
	hitmap_change_for( map, 15, BIT_SET );
	hitmap_change_for( map, 63, BIT_SET );
	hitmap_change_for( map, 127, BIT_SET );
	hitmap_change_for( map, 1ul << ( 2 * _WORD_POW - 1 ), BIT_SET );
	hitmap_change_for( map, 1ul << ( 2 * _WORD_POW ), BIT_SET );
	hitmap_change_for( map,
		( 1ul << ( 2 * _WORD_POW + 1 ) ) - 1,
		BIT_SET
	);

	ck_assert_int_eq( hitmap_discover( map, 0, BIT_SET ), 0 );
	ck_assert_int_eq( hitmap_discover( map, 8, BIT_SET ), 15 );
	ck_assert_int_eq( hitmap_discover( map, 34, BIT_SET ), 63 );
	ck_assert_int_eq( hitmap_discover( map, 66, BIT_SET ), 127 );
	ck_assert_int_eq(
		hitmap_discover( map, 150, BIT_SET ),
		1ul << ( 2 * _WORD_POW - 1 )
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << ( 2 * _WORD_POW - 1 ) ) + 1, BIT_SET ),
		1ul << ( 2 * _WORD_POW )
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << ( 2 * _WORD_POW ) ) + 1, BIT_SET ),
		( 1ul << ( 2 * _WORD_POW + 1 ) ) - 1
	);

	// 2 layers + 1 layer with 4 words + 1 layer with one word
	hitmap_init( map, 3 * _WORD_POW + 3 );

	ck_assert_int_eq(
		hitmap_discover( map, 0, BIT_SET ), SIZE_MAX
	);
	ck_assert_int_eq(
		hitmap_discover( map,
			( 1ul << ( 3 * _WORD_POW + 2 ) ) - 1,
			BIT_SET
		),
		SIZE_MAX
	);

	ck_assert_int_eq( hitmap_discover( map, 0, BIT_UNSET ), 0 );
	ck_assert_int_eq(
		hitmap_discover( map,
			( 1ul << ( 3 * _WORD_POW + 2 ) ) - 1,
			BIT_UNSET
		),
		( 1ul << ( 3 * _WORD_POW + 2 ) ) - 1
	);

	hitmap_change_for( map, 32768, BIT_SET );
	ck_assert_int_eq(
		hitmap_discover( map, 0, BIT_SET ),
		32768
	);
	hitmap_change_for( map, 32768, BIT_UNSET );

	struct tms tprev;
	times( &tprev );
	for( size_t cyc = 0; cyc < ( 1ul << ( 3 * _WORD_POW + 3 ) ); ++cyc ) {
		hitmap_change_for( map, cyc, BIT_SET );
		ck_assert_int_eq(
			hitmap_discover( map, 0, BIT_SET ),
			cyc
		);
		hitmap_change_for( map, cyc, BIT_UNSET );
	}
	struct tms tnow;
	times( &tnow );
	printf( "Hitmap time: %d\n", tnow.tms_utime - tprev.tms_utime );

	AO_t *dummy_map = malloc(
		( 1ul << ( 2 * _WORD_POW + 3 ) ) * sizeof( AO_t )
	);
	times( &tprev );
	for( size_t cyc = 0; cyc < ( 1ul << ( 3 * _WORD_POW + 3 ) ); ++cyc ) {
		dummy_change_for( dummy_map,
			3 * _WORD_POW + 3,
			cyc,
			BIT_SET
		);
		ck_assert_int_eq(
			dummy_discover( dummy_map,
				3 * _WORD_POW + 3,
				0,
				BIT_SET
			),
			cyc
		);
		dummy_change_for( dummy_map,
			3 * _WORD_POW + 3,
			cyc,
			BIT_UNSET
		);
	}
	times( &tnow );
	free( dummy_map );
	printf( "Dummy map time: %d\n", tnow.tms_utime - tprev.tms_utime );

	/*hitmap_change_for( map, 0, BIT_SET );
	hitmap_change_for( map, 15, BIT_SET );
	hitmap_change_for( map, ( 1ul << _WORD_POW ) + 1, BIT_SET );
	hitmap_change_for( map, 1ul << ( _WORD_POW + 1 ), BIT_SET );
	hitmap_change_for( map, 1ul << ( _WORD_POW + 2 ), BIT_SET );
	hitmap_change_for( map, ( 1ul << ( 2 * _WORD_POW ) ) + 1 , BIT_SET );
	hitmap_change_for( map,
		( 1ul << ( 3 * _WORD_POW + 3 ) ) - 1,
		BIT_SET
	);

	ck_assert_int_eq( hitmap_discover( map, 0, BIT_SET ), 0 );
	ck_assert_int_eq( hitmap_discover( map, 1, BIT_SET ), 15 );
	ck_assert_int_eq(
		hitmap_discover( map, 16, BIT_SET ),
		( 1ul << _WORD_POW ) + 1
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << _WORD_POW ) + 1, BIT_SET ),
		1ul << ( _WORD_POW + 1 )
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << ( _WORD_POW + 1 ) ) + 1, BIT_SET ),
		1ul << ( _WORD_POW + 2 )
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << ( _WORD_POW + 2 ) ) + 1, BIT_SET ),
		( 1ul << ( 2 * _WORD_POW ) ) + 1
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << ( 2 * _WORD_POW ) ) + 2, BIT_SET ),
		( 1ul << ( 3 * _WORD_POW + 3 ) ) - 1
	);
	for( size_t cyc = 0; cyc < ( 1ul << ( _WORD_POW + 3 ) ); ++cyc )
		hitmap_change_for( map, cyc, BIT_SET );

	ck_assert_int_eq(
		hitmap_discover( map, 0, BIT_UNSET ), SIZE_MAX
	);

	hitmap_change_for( map, 0, BIT_UNSET );
	hitmap_change_for( map, 1, BIT_UNSET );
	hitmap_change_for( map, 3, BIT_UNSET );
	hitmap_change_for( map, 7, BIT_UNSET );
	hitmap_change_for( map, 15, BIT_UNSET );
	hitmap_change_for( map, 31, BIT_UNSET );
	hitmap_change_for( map, 33, BIT_UNSET );
	hitmap_change_for( map, 63, BIT_UNSET );
	hitmap_change_for( map, 127, BIT_UNSET );
	hitmap_change_for( map,
		( 1ul << ( _WORD_POW + 3 ) ) - 1,
		BIT_UNSET
	);

	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 0, BIT_UNSET ), 0 );
	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 1, BIT_UNSET ), 1 );
	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 2, BIT_UNSET ), 3 );
	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 5, BIT_UNSET ), 7 );
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 8, BIT_UNSET ), 15
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 19, BIT_UNSET ), 31
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 32, BIT_UNSET ), 33
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 34, BIT_UNSET ), 63
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 66, BIT_UNSET ), 127
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 150, BIT_UNSET ),
		( 1ul << ( _WORD_POW + 3 ) ) - 1
	);*/

	free( map );
}
END_TEST
/*
START_TEST( test_calc_sz_border )
{
	ck_assert_int_eq( hitmap_calc_sz( 1 ), sizeof( map_t ) + sizeof( AO_t ) );
}
END_TEST

START_TEST( test_init_border )
{
	AO_t map[ 3 ];
	map[ 0 ] = 3;
	map[ 1 ] = 1;
	map[ 2 ] = 2;
	hitmap_init( map, _WORD_POW );

	ck_assert_int_eq( map[ 1 ], 1 );
	ck_assert_int_eq( map[ 2 ], 2 );
	ck_assert_int_eq( map[ 0 ], 0 );
}
END_TEST

START_TEST( test_change_for_border )
{
	AO_t map[ 3 ];
	map[ 0 ] = 3;
	map[ 1 ] = 2121;
	map[ 2 ] = 1212;
	hitmap_init( map, _WORD_POW );

	hitmap_change_for( map, _WORD_POW, ( 1ul << _WORD_POW ) - 1, BIT_SET );

	ck_assert_int_eq( map[ 0 ], 1ul << ( ( 1ul << _WORD_POW ) - 1 ) );
	ck_assert_int_eq( map[ 1 ], 2121 );
	ck_assert_int_eq( map[ 2 ], 1212 );

	hitmap_change_for( map, _WORD_POW, ( 1ul << _WORD_POW ) - 1, BIT_UNSET );

	ck_assert_int_eq( map[ 0 ], 0 );
	ck_assert_int_eq( map[ 1 ], 2121 );
	ck_assert_int_eq( map[ 2 ], 1212 );
}
END_TEST

START_TEST( test_has_border )
{
	AO_t map[ 1 ];
	hitmap_init( map, _WORD_POW );

	ck_assert( ! hitmap_has( map, _WORD_POW, BIT_SET ) );
	for( size_t idx = 0; idx < ( 1ul << _WORD_POW ); ++idx ) {
		ck_assert( hitmap_has( map, _WORD_POW, BIT_UNSET ) );
		hitmap_change_for( map, _WORD_POW, idx, BIT_SET );
		ck_assert( hitmap_has( map, _WORD_POW, BIT_SET ) );
	}

	ck_assert( ! hitmap_has( map, _WORD_POW, BIT_UNSET ) );
}
END_TEST

START_TEST( test_discover_border )
{
	AO_t map[ 1 ];
	hitmap_init( map, _WORD_POW );

	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW, 0, BIT_SET ),
		SIZE_MAX
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW, ( 1ul << _WORD_POW ) - 1, BIT_SET ),
		SIZE_MAX
	);

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc ) {
		hitmap_change_for( map, _WORD_POW, cyc, BIT_SET );
		ck_assert_int_eq( hitmap_discover( map, _WORD_POW, 0, BIT_SET ), cyc );
		hitmap_change_for( map, _WORD_POW, cyc, BIT_UNSET );
	}

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc )
		hitmap_change_for( map, _WORD_POW, cyc, BIT_SET );

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc ) {
		hitmap_change_for( map, _WORD_POW, cyc, BIT_UNSET );
		ck_assert_int_eq(
			hitmap_discover( map, _WORD_POW, 0, BIT_UNSET ), cyc
		);
		hitmap_change_for( map, _WORD_POW, cyc, BIT_SET );
	}
}
END_TEST
*/
Suite *hitmap_suite( void ) {
	Suite *s = suite_create( "Hitmap" );

	TCase *tc_basic = tcase_create( "Basic" );
	tcase_add_test( tc_basic, test_calc_sz_normal );
	tcase_add_test( tc_basic, test_init_normal );
	tcase_add_test( tc_basic, test_change_for_normal );
	//tcase_add_test( tc_basic, test_has_normal );
	tcase_add_test( tc_basic, test_discover_normal );
	
	TCase *tc_border = tcase_create( "Border" );
	//tcase_add_test( tc_border, test_init_border );
	//tcase_add_test( tc_border, test_change_for_border );
	//tcase_add_test( tc_border, test_has_border );
	//tcase_add_test( tc_border, test_discover_border );
	//tcase_add_test( tc_border, test_calc_sz_border );

	suite_add_tcase( s, tc_basic );
	suite_add_tcase( s, tc_border );

	return s;
}

int main( void ) {
	int number_failed;

	SRunner *sr = srunner_create( hitmap_suite() );
	srunner_run_all( sr, CK_NORMAL );
	number_failed = srunner_ntests_failed( sr );
	srunner_free( sr );
	
	return ( number_failed == 0 ) ? EXIT_SUCCESS : EXIT_FAILURE;
}
