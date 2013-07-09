#ifndef LIBHITMAP
#define LIBHITMAP

#include <atomic_ops.h>
#include <limits.h>

#define HITMAP_INITIAL_SIZE ( sizeof( AO_t ) * 8 )

#if ( ULONG_MAX != 4294967295UL )
	#define _WORD_POW ( 6 )
#else
	#define _WORD_POW ( 5 )
#endif

enum hitmap_mark {
	BIT_UNSET = 0,
	BIT_SET = 1
};

enum hitmap_find {
	FIND_UNSET = 1,
	FIND_SET = 0
};

/**
 * Calculates size of bitmap in bytes.
 * Calculates actual size of bitmap structure in AO_t from given
 * "power of capacity". Power of capacity is the power which two should be
 * raised to get requested bitmap capacity (number of bits in bitmap). The fact
 * follows from the description is that capacity can be only power of two.
 * @param len_pow binary logarithm from requested number of bits
 * @return number of AO_t should be allocated for map with requested length
 * @see hitmap_init
 */
inline static size_t hitmap_calc_sz( int len_pow ) {
	if( len_pow < _WORD_POW )
		return 1;

	return 3 * ( 1 << ( len_pow - _WORD_POW ) ) - 2;
}

/**
 * Initializes allocated map for further use.
 * Allocation of bitmap should be handled by user. Library provides function for
 * appropriate initialization only. Firstly, size of map should be calculated
 * with help of hitmap_calc_sz and passed to allocator (or as array length in
 * the case of in-stack allocation). Then, allocated array should be
 * initialized with help of hitmap_init. For simplicity, no special structure
 * is used for bitmap - it's just array of AO_t. That's why user needs to pass
 * len_pow parameter for each library function.
 * @param map allocated bitmap (simple C array)
 * @param len_pow binary logarithm derived from requested number of bits
 * @see hitmap_calc_sz
 */
extern void hitmap_init( AO_t map[], int len_pow );

/**
 * Sets or unsets bit with specified index.
 * According to passed is_put operation selector, sets or unsets bit in map
 * with specified index.
 * @param map allocated bitmap (simple C array)
 * @param len_pow binary logarithm derived from requested number of bits
 * @param idx linear index of bit in map
 * @param is_put operation selector (set/unset)
 * @see hitmap_discover
 */
extern void hitmap_change_for( AO_t map[],
	int len_pow,
	size_t idx,
	enum hitmap_mark is_put
);

/**
 * Looks for set/unset bit starting from specified index.
 * According to passed which operation selector, looks for set/unset bit in map
 * starting from specified index inclusevly.
 * @param map allocated bitmap (simple C array)
 * @param len_pow binary logarithm derived from requested number of bits
 * @param start_idx starting point of search
 * @param which operation selector (look for set/unset bit)
 * @return index of first occurence of set/unset bit
 * @see hitmap_change_for
 * @see hitmap_has
 */
extern size_t hitmap_discover( AO_t map[],
	int len_pow,
	size_t start_idx,
	enum hitmap_find which
);

/**
 * Tests if specified map has set/unset positions.
 * According to passed which operation selector, tests for presence of set/unset
 * bits in specified map.
 * @param map allocated bitmap (simple C array)
 * @param len_pow binary logarithm derived from requested number of bits
 * @param which operation selector (look for set/unset bit)
 * @return != 0 - if map has specified kind of bits; == 0 - otherwise
 * @see hitmap_change_for
 * @see hitmap_discover
 */
extern int hitmap_has( AO_t map[], int len_pow, enum hitmap_mark which );

#endif
