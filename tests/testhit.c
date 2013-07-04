#include <check.h>
#include <hitmap.h>

#include <stdlib.h>
#include <stdint.h>

START_TEST( test_calc_sz )
{
	ck_assert_int_eq( hitmap_calc_sz( 5 ), 1 );
	ck_assert_int_eq( hitmap_calc_sz( 6 ), 4 );
	ck_assert_int_eq( hitmap_calc_sz( 7 ), 10 );
	ck_assert_int_eq( hitmap_calc_sz( 8 ), 22 );
}
END_TEST

START_TEST( test_init )
{
	size_t sz = hitmap_calc_sz( 8 ),
		capacity = ( 1ul << ( 8 - _WORD_POW ) );
	AO_t *map = malloc( ( sz + 2 ) * sizeof( AO_t ) );
	for( size_t cyc = 0; cyc < sz; ++cyc )
		map[ cyc ] = 3;
	map[ sz ] = 1;
	map[ sz + 1 ] = 2;
	hitmap_init( map, 8 );

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
}
END_TEST

START_TEST( test_change_for )
{
	size_t sz = hitmap_calc_sz( 8 );
	AO_t *map = malloc( sz * sizeof( AO_t ) );
	hitmap_init( map, 8 );

	hitmap_change_for( map, 8, 0, BIT_SET );
	hitmap_change_for( map, 8, 2, BIT_SET );
	hitmap_change_for( map, 8, 4, BIT_SET );

	ck_assert_int_eq( map[ 0 ], 21 );

	size_t start_with = 1ul << ( 8 - _WORD_POW );
	ck_assert_int_eq( map[ start_with ], 7 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 8 - _WORD_POW - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 8 - _WORD_POW - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	
	hitmap_change_for( map, 8, 1, BIT_SET );

	start_with = 1ul << ( 8 - _WORD_POW );
	ck_assert_int_eq( map[ start_with ], 7 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX - 1 );
	start_with += ( 1ul << ( 8 - _WORD_POW - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
	start_with += ( 1ul << ( 8 - _WORD_POW - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );

	hitmap_change_for( map, 8, 3, BIT_SET );
	hitmap_change_for( map, 8, 5, BIT_SET );
	hitmap_change_for( map, 8, 6, BIT_SET );

	start_with = 1ul << ( 8 - _WORD_POW );
	ck_assert_int_eq( map[ start_with ], 15 );
	ck_assert_int_eq( map[ start_with + 1 ], ( SIZE_MAX ^ 7ul ) );
	start_with += ( 1ul << ( 8 - _WORD_POW - 1 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 3 );
	ck_assert_int_eq( map[ start_with + 1 ], ( SIZE_MAX ^ 1ul ) );
	start_with += ( 1ul << ( 8 - _WORD_POW - 2 ) ) * 2;
	ck_assert_int_eq( map[ start_with ], 1 );
	ck_assert_int_eq( map[ start_with + 1 ], SIZE_MAX );
}
END_TEST

Suite *hitmap_suite( void ) {
	Suite *s = suite_create( "Hitmap" );

	TCase *tc_basic = tcase_create( "Basic" );
	tcase_add_test( tc_basic, test_calc_sz );
	tcase_add_test( tc_basic, test_init );
	tcase_add_test( tc_basic, test_change_for );
	suite_add_tcase( s, tc_basic );

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
