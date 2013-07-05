#include <check.h>
#include <hitmap.h>

#include <stdlib.h>
#include <stdint.h>

START_TEST( test_calc_sz_normal )
{
	ck_assert_int_eq( hitmap_calc_sz( _WORD_POW ), 1 );
	ck_assert_int_eq( hitmap_calc_sz( _WORD_POW + 1 ), 4 );
	ck_assert_int_eq( hitmap_calc_sz( _WORD_POW + 2 ), 10 );
	ck_assert_int_eq( hitmap_calc_sz( _WORD_POW + 3 ), 22 );
}
END_TEST

START_TEST( test_init_normal )
{
	size_t sz = hitmap_calc_sz( _WORD_POW + 3 ),
		capacity = ( 1ul << 3 );
	AO_t *map = malloc( ( sz + 2 ) * sizeof( AO_t ) );
	for( size_t cyc = 0; cyc < sz; ++cyc )
		map[ cyc ] = 3;
	map[ sz ] = 1;
	map[ sz + 1 ] = 2;
	hitmap_init( map, _WORD_POW + 3 );

	ck_assert_int_eq( map[ sz ], 1 );
	ck_assert_int_eq( map[ sz + 1 ], 2 );

	for( size_t cyc = 0; cyc < capacity; ++cyc )
		ck_assert_int_eq( map[ cyc ], 0 );

	size_t start_from = capacity;
	capacity >>= 1;

	int is_zero = 0;
	for( size_t cyc = 0; cyc < ( capacity * 2 ); ++cyc ) {
		is_zero = ! is_zero;

		if( is_zero )
			ck_assert_int_eq( map[ start_from + cyc ], 0 );
		else
			ck_assert_int_eq( map[ start_from + cyc ], SIZE_MAX );
	}

	ck_assert( ! is_zero );

	start_from += capacity * 2;
	capacity >>= 1;
	is_zero = 0;
	for( size_t cyc = 0; cyc < ( capacity * 2 ); ++cyc ) {
		is_zero = ! is_zero;

		if( is_zero )
			ck_assert_int_eq( map[ start_from + cyc ], 0 );
		else
			ck_assert_int_eq( map[ start_from + cyc ], SIZE_MAX );
	}

	ck_assert( ! is_zero );

	start_from += capacity * 2;
	ck_assert_int_eq( map[ start_from ], 0 );
	ck_assert_int_eq( map[ start_from + 1 ], SIZE_MAX );
	
	ck_assert_int_eq( start_from + 2, sz );

	free( map );
}
END_TEST

START_TEST( test_change_for_normal )
{
	size_t sz = hitmap_calc_sz( _WORD_POW + 3 );
	AO_t *map = malloc( ( sz + 2 ) * sizeof( AO_t ) );
	map[ sz ] = 2121;
	map[ sz + 1 ] = 1212;
	hitmap_init( map, _WORD_POW + 3 );

	hitmap_change_for( map, _WORD_POW + 3, 0, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 2, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 4, BIT_SET );

	ck_assert_int_eq( map[ 0 ], 0x15 );

	size_t start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ], 7 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	
	hitmap_change_for( map, _WORD_POW + 3, 1, BIT_SET );

	ck_assert_int_eq( map[ 0 ], 0x17 );
	
	start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ], 7 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX - 1 );
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, _WORD_POW + 3, 3, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 5, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 6, BIT_SET );

	ck_assert_int_eq( map[ 0 ], 0x7F );

	start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ], 15 );
	ck_assert_int_eq( map[ start_with + 1 ], ( SIZE_MAX ^ 7ul ) );
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], ( SIZE_MAX ^ 1ul ) );
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ), BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 1, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 3, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 4, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 5, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 6, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 7, BIT_SET );

	ck_assert_int_eq( map[ 0 ], 0x7F );
	ck_assert_int_eq( map[ 1 ], 0xFB );

	start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ],
		0x0F | ( 0x0F << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	ck_assert_int_eq( map[ start_with + 1 ],
		SIZE_MAX ^ 7ul ^ ( 0x0D << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ],
		0x03 | ( 0x03 << ( 1ul << ( _WORD_POW - 2 ) ) )
	);
	ck_assert_int_eq( map[ start_with + 1 ],
		SIZE_MAX ^ 1ul ^ ( 0x02 << ( 1ul << ( _WORD_POW - 2 ) ) )
	);
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ],
		0x01 | ( 0x01 << ( 1ul << ( _WORD_POW - 3 ) ) )
	);
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 2, BIT_SET );

	ck_assert_int_eq( map[ start_with + 1 ],
		SIZE_MAX ^ ( 0x01 << ( 1ul << ( _WORD_POW - 3 ) ) )
	);

	ck_assert_int_eq( map[ sz ], 2121 );
	ck_assert_int_eq( map[ sz + 1 ], 1212 );

	hitmap_change_for( map, _WORD_POW + 3,
		( 1ul << _WORD_POW ) + 2, BIT_UNSET
	);

	ck_assert_int_eq( map[ 0 ], 0x7F );
	ck_assert_int_eq( map[ 1 ], 0xFB );
	start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ],
		0x0F | ( 0x0F << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	ck_assert_int_eq( map[ start_with + 1 ],
		SIZE_MAX ^ 7ul ^ ( 0x0D << ( 1ul << ( _WORD_POW - 1 ) ) )
	);
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ],
		0x03 | ( 0x03 << ( 1ul << ( _WORD_POW - 2 ) ) )
	);
	ck_assert_int_eq( map[ start_with + 1 ],
		SIZE_MAX ^ 1ul ^ ( 0x02 << ( 1ul << ( _WORD_POW - 2 ) ) )
	);
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ],
		0x01 | ( 0x01 << ( 1ul << ( _WORD_POW - 3 ) ) )
	);
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ), BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 1,
		BIT_UNSET
	);
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 3,
		BIT_UNSET
	);
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 4,
		BIT_UNSET
	);
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 5,
		BIT_UNSET
	);
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 6,
		BIT_UNSET
	);
	hitmap_change_for( map, _WORD_POW + 3, ( 1ul << _WORD_POW ) + 7,
		BIT_UNSET
	);

	ck_assert_int_eq( map[ 0 ], 0x7F );

	start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ], 15 );
	ck_assert_int_eq( map[ start_with + 1 ], ( SIZE_MAX ^ 7ul ) );
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], ( SIZE_MAX ^ 1ul ) );
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, _WORD_POW + 3, 3, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 5, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 6, BIT_UNSET );

	ck_assert_int_eq( map[ 0 ], 0x17 );
	
	start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ], 7 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX - 1 );
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, _WORD_POW + 3, 1, BIT_UNSET );

	ck_assert_int_eq( map[ 0 ], 0x15 );

	start_with = 1ul << 3;
	ck_assert_int_eq( map[ start_with ], 7 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 3 - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 3 - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	ck_assert_int_eq( map[ sz ], 2121 );
	ck_assert_int_eq( map[ sz + 1 ], 1212 );

	free( map );
}
END_TEST

START_TEST( test_has_normal )
{
	AO_t *map = malloc( hitmap_calc_sz( _WORD_POW + 3 ) * sizeof( AO_t ) );
	hitmap_init( map, _WORD_POW + 3 );

	ck_assert( ! hitmap_has( map, _WORD_POW + 3, BIT_SET ) );
	for( size_t idx = 0; idx < ( 1 << ( _WORD_POW + 3 ) ); ++idx ) {
		ck_assert( hitmap_has( map, _WORD_POW + 3, BIT_UNSET ) );
		hitmap_change_for( map, _WORD_POW + 3, idx, BIT_SET );
		ck_assert( hitmap_has( map, _WORD_POW + 3, BIT_SET ) );
	}

	ck_assert( ! hitmap_has( map, _WORD_POW + 3, BIT_UNSET ) );

	free( map );
}
END_TEST

START_TEST( test_discover_normal )
{
	AO_t *map = malloc( hitmap_calc_sz( _WORD_POW + 3 ) * sizeof( AO_t ) );
	hitmap_init( map, _WORD_POW + 3 );

	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 0, FIND_SET ), SIZE_MAX
	);
	ck_assert_int_eq(
		hitmap_discover( map,
			_WORD_POW + 3,
			( 1ul << ( _WORD_POW + 3 ) ) - 1,
			FIND_SET
		),
		SIZE_MAX
	);

	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 0, FIND_UNSET ), 0 );
	ck_assert_int_eq(
		hitmap_discover( map,
			_WORD_POW + 3,
			( 1ul << ( _WORD_POW + 3 ) ) - 1,
			FIND_UNSET
		),
		( 1ul << ( _WORD_POW + 3 ) ) - 1
	);

	for( size_t cyc = 0; cyc < ( 1ul << ( _WORD_POW + 3 ) ); ++cyc ) {
		hitmap_change_for( map, _WORD_POW + 3, cyc, BIT_SET );
		ck_assert_int_eq(
			hitmap_discover( map, _WORD_POW + 3, 0, FIND_SET ),
			cyc
		);
		hitmap_change_for( map, _WORD_POW + 3, cyc, BIT_UNSET );
	}

	hitmap_change_for( map, _WORD_POW + 3, 0, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 1, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 3, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 7, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 15, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 31, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 33, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 63, BIT_SET );
	hitmap_change_for( map, _WORD_POW + 3, 127, BIT_SET );
	hitmap_change_for( map,
		_WORD_POW + 3,
		( 1ul << ( _WORD_POW + 3 ) ) - 1,
		BIT_SET
	);

	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 0, FIND_SET ), 0
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 1, FIND_SET ), 1
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 2, FIND_SET ), 3
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 5, FIND_SET ), 7
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 8, FIND_SET ), 15
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 19, FIND_SET ), 31
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 32, FIND_SET ), 33
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 34, FIND_SET ), 63
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 66, FIND_SET ), 127
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 150, FIND_SET ),
		( 1ul << ( _WORD_POW + 3 ) ) - 1
	);

	for( size_t cyc = 0; cyc < ( 1ul << ( _WORD_POW + 3 ) ); ++cyc )
		hitmap_change_for( map, _WORD_POW + 3, cyc, BIT_SET );

	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 0, FIND_UNSET ), SIZE_MAX
	);

	hitmap_change_for( map, _WORD_POW + 3, 0, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 1, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 3, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 7, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 15, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 31, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 33, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 63, BIT_UNSET );
	hitmap_change_for( map, _WORD_POW + 3, 127, BIT_UNSET );
	hitmap_change_for( map,
		_WORD_POW + 3,
		( 1ul << ( _WORD_POW + 3 ) ) - 1,
		BIT_UNSET
	);

	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 0, FIND_UNSET ), 0 );
	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 1, FIND_UNSET ), 1 );
	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 2, FIND_UNSET ), 3 );
	ck_assert_int_eq( hitmap_discover( map, _WORD_POW + 3, 5, FIND_UNSET ), 7 );
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 8, FIND_UNSET ), 15
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 19, FIND_UNSET ), 31
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 32, FIND_UNSET ), 33
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 34, FIND_UNSET ), 63
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 66, FIND_UNSET ), 127
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW + 3, 150, FIND_UNSET ),
		( 1ul << ( _WORD_POW + 3 ) ) - 1
	);

	free( map );
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
		hitmap_discover( map, _WORD_POW, 0, FIND_SET ),
		SIZE_MAX
	);
	ck_assert_int_eq(
		hitmap_discover( map, _WORD_POW, ( 1ul << _WORD_POW ) - 1, FIND_SET ),
		SIZE_MAX
	);

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc ) {
		hitmap_change_for( map, _WORD_POW, cyc, BIT_SET );
		ck_assert_int_eq( hitmap_discover( map, _WORD_POW, 0, FIND_SET ), cyc );
		hitmap_change_for( map, _WORD_POW, cyc, BIT_UNSET );
	}

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc )
		hitmap_change_for( map, _WORD_POW, cyc, BIT_SET );

	for( size_t cyc = 0; cyc < ( 1ul << _WORD_POW ); ++cyc ) {
		hitmap_change_for( map, _WORD_POW, cyc, BIT_UNSET );
		ck_assert_int_eq(
			hitmap_discover( map, _WORD_POW, 0, FIND_UNSET ), cyc
		);
		hitmap_change_for( map, _WORD_POW, cyc, BIT_SET );
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
	
	TCase *tc_border = tcase_create( "Border" );
	tcase_add_test( tc_border, test_init_border );
	tcase_add_test( tc_border, test_change_for_border );
	tcase_add_test( tc_border, test_has_border );
	tcase_add_test( tc_border, test_discover_border );

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
