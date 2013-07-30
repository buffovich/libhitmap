#include <check.h>
#include <hitmap.h>

#include <stdlib.h>
#include <stdint.h>
#include <alloca.h>

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
		sizeof( map_t ) + 4 * sizeof( AO_t )
	);
	ck_assert_int_eq(
		hitmap_calc_sz( _WORD_POW + 3 ),
		sizeof( map_t ) + 8 * sizeof( AO_t )
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

	( ( char* ) ( map->bits +
		( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
	) )[ 0 ] = 111;
	( ( char* ) ( map->bits +
		( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
	) )[ 1 ] = 112;
	hitmap_init( map, _DUMMY_THRESHOLD_POW );
	ck_assert_int_eq(
		( ( char* ) ( map->bits +
				( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
			) )[ 0 ],
		111
	);
	ck_assert_int_eq(
		( ( char* ) ( map->bits +
				( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
			) )[ 1 ],
		112
	);

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

	hitmap_init( map, _DUMMY_THRESHOLD_POW );

	( ( char* ) ( map->bits +
		( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
	) )[ 0 ] = 111;
	( ( char* ) ( map->bits +
		( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
	) )[ 1 ] = 112;
	hitmap_change_for( map, 3, BIT_SET );
	ck_assert_int_eq( map->bits[ 0 ], 8 );
	ck_assert_int_eq(
		( ( char* ) ( map->bits +
				( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
			) )[ 0 ],
		111
	);
	ck_assert_int_eq(
		( ( char* ) ( map->bits +
				( 1ul << ( _DUMMY_THRESHOLD_POW - _WORD_POW ) )
			) )[ 1 ],
		112
	);

	free( map );
}
END_TEST


START_TEST( test_has_normal )
{
	map_t *map = malloc( hitmap_calc_sz( 3 * _WORD_POW + 2 ) );

	for( int pow = _WORD_POW; pow < ( 3 * _WORD_POW + 2 ); ++pow ) {
		hitmap_init( map, pow );

		ck_assert( ! hitmap_has( map, BIT_SET ) );
		for( size_t idx = 0; idx < ( 1ul << pow ); ++idx ) {
			ck_assert( hitmap_has( map, BIT_UNSET ) );
			hitmap_change_for( map, idx, BIT_SET );
			ck_assert( hitmap_has( map, BIT_SET ) );
		}

		ck_assert( ! hitmap_has( map, BIT_UNSET ) );
	}

	free( map );
}
END_TEST

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

	for( size_t cyc = 0; cyc < ( 1ul << ( 3 * _WORD_POW + 3 ) ); ++cyc ) {
		hitmap_change_for( map, cyc, BIT_SET );
		ck_assert_int_eq(
			hitmap_discover( map, 0, BIT_SET ),
			cyc
		);
		hitmap_change_for( map, cyc, BIT_UNSET );
	}

	for( int cyc = 3 * _WORD_POW + 3; cyc >= _WORD_POW; --cyc ) {
		hitmap_init( map, cyc );
		hitmap_change_for( map, 29, BIT_SET );
		hitmap_change_for( map, 31, BIT_SET );
		ck_assert_int_eq(
			hitmap_discover( map, 30, BIT_SET ),
			31
		);
		hitmap_change_for( map, 29, BIT_UNSET );
		hitmap_change_for( map, 31, BIT_UNSET );
		for( size_t cyc2 = 0; cyc2 < ( 1ul << cyc ); ++cyc2 ) {
			hitmap_change_for( map, cyc2, BIT_SET );
			ck_assert_int_eq(
				hitmap_discover( map, 0, BIT_SET ),
				cyc2
			);
			hitmap_change_for( map, cyc2, BIT_UNSET );
		}
	}

	for( int cyc = 3 * _WORD_POW + 3; cyc >= _WORD_POW; --cyc ) {
		hitmap_init( map, cyc );
		for( size_t cyc2 = 0; cyc2 < ( 1ul << cyc ); ++cyc2 )
			hitmap_change_for( map, cyc2, BIT_SET );

		hitmap_change_for( map, 29, BIT_UNSET );
		hitmap_change_for( map, 31, BIT_UNSET );
		ck_assert_int_eq(
			hitmap_discover( map, 30, BIT_UNSET ),
			31
		);
		hitmap_change_for( map, 29, BIT_SET );
		hitmap_change_for( map, 31, BIT_SET );

		ck_assert_int_eq( hitmap_discover( map, 0, BIT_UNSET ), SIZE_MAX );

		for( size_t cyc2 = 0; cyc2 < ( 1ul << cyc ); ++cyc2 ) {
			hitmap_change_for( map, cyc2, BIT_UNSET );
			ck_assert_int_eq(
				hitmap_discover( map, 0, BIT_UNSET ),
				cyc2
			);
			hitmap_change_for( map, cyc2, BIT_SET );
		}
	}

	free( map );
}
END_TEST

START_TEST( test_calc_sz_border )
{
	ck_assert_int_eq( hitmap_calc_sz( 1 ), sizeof( map_t ) + sizeof( AO_t ) );
}
END_TEST

START_TEST( test_init_border )
{
	map_t *map = alloca( hitmap_calc_sz( _WORD_POW  ) + sizeof( AO_t ) * 2 );
	map->bits[ 0 ] = 3;
	map->bits[ 1 ] = 1;
	map->bits[ 2 ] = 2;
	hitmap_init( map, _WORD_POW );
	ck_assert_int_eq( map->bits_border, NULL );

	ck_assert_int_eq( map->bits[ 1 ], 1 );
	ck_assert_int_eq( map->bits[ 2 ], 2 );
	ck_assert_int_eq( map->bits[ 0 ], 0 );
}
END_TEST

START_TEST( test_change_for_border )
{
	map_t *map = alloca( hitmap_calc_sz( _WORD_POW  ) + sizeof( AO_t ) * 2 );
	map->bits[ 0 ] = 3;
	map->bits[ 1 ] = 2121;
	map->bits[ 2 ] = 1212;
	hitmap_init( map, _WORD_POW );

	hitmap_change_for( map, ( 1ul << _WORD_POW ) - 1, BIT_SET );

	ck_assert_int_eq( map->bits[ 0 ], 1ul << ( ( 1ul << _WORD_POW ) - 1 ) );
	ck_assert_int_eq( map->bits[ 1 ], 2121 );
	ck_assert_int_eq( map->bits[ 2 ], 1212 );

	hitmap_change_for( map, ( 1ul << _WORD_POW ) - 1, BIT_UNSET );

	ck_assert_int_eq( map->bits[ 0 ], 0 );
	ck_assert_int_eq( map->bits[ 1 ], 2121 );
	ck_assert_int_eq( map->bits[ 2 ], 1212 );
}
END_TEST

START_TEST( test_has_border )
{
	map_t *map = alloca( hitmap_calc_sz( _WORD_POW ) );
	hitmap_init( map, _WORD_POW );

	ck_assert( ! hitmap_has( map, BIT_SET ) );
	for( size_t idx = 0; idx < ( 1ul << _WORD_POW ); ++idx ) {
		ck_assert( hitmap_has( map, BIT_UNSET ) );
		hitmap_change_for( map, idx, BIT_SET );
		ck_assert( hitmap_has( map, BIT_SET ) );
	}

	ck_assert( ! hitmap_has( map, BIT_UNSET ) );
}
END_TEST

START_TEST( test_discover_border )
{
	map_t *map = alloca( hitmap_calc_sz( _WORD_POW ) );
	hitmap_init( map, _WORD_POW );

	ck_assert_int_eq(
		hitmap_discover( map, 0, BIT_SET ),
		SIZE_MAX
	);
	ck_assert_int_eq(
		hitmap_discover( map, ( 1ul << _WORD_POW ) - 1, BIT_SET ),
		SIZE_MAX
	);

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc ) {
		hitmap_change_for( map, cyc, BIT_SET );
		ck_assert_int_eq( hitmap_discover( map, 0, BIT_SET ), cyc );
		hitmap_change_for( map, cyc, BIT_UNSET );
	}

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc )
		hitmap_change_for( map, cyc, BIT_SET );

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc ) {
		hitmap_change_for( map, cyc, BIT_UNSET );
		ck_assert_int_eq(
			hitmap_discover( map, 0, BIT_UNSET ), cyc
		);
		hitmap_change_for( map, cyc, BIT_SET );
	}
}
END_TEST

Suite *hitmap_suite( void ) {
	Suite *s = suite_create( "Hitmap" );

	TCase *tc_basic = tcase_create( "Basic" );
	tcase_add_test( tc_basic, test_calc_sz_normal );
	tcase_add_test( tc_basic, test_init_normal );
	tcase_add_test( tc_basic, test_change_for_normal );
	tcase_add_test( tc_basic, test_has_normal );
	tcase_add_test( tc_basic, test_discover_normal );
	tcase_set_timeout( tc_basic, 0 );
	
	TCase *tc_border = tcase_create( "Border" );
	tcase_add_test( tc_border, test_init_border );
	tcase_add_test( tc_border, test_change_for_border );
	tcase_add_test( tc_border, test_has_border );
	tcase_add_test( tc_border, test_discover_border );
	tcase_add_test( tc_border, test_calc_sz_border );

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
