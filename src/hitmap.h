/**
 * @file hitmap.h
 * @brief Hitmap interface.
 *
 * File describes public library API.
 */

/**
 * @page intro Introduction
 *
 * Hierarchical bitmap (bit vector) structure (in short - "hitmap") provides
 * an user with:
 * 
 * - compact view of boolean vector;
 * 
 *   You need only 40 (32-bit)/48 (64-bit) bytes for 256 booleans and
 *   144 (on 32-bit)/ 160 (on 64-bit) for 1024 booleans representation.
 * 
 * - O( log32( n ) )/O( log64( n ) ) complexity of looking for the next
 *   set/unset bit index;
 * - O( 1 ) complexity of defining if map has set/unset bits;
 * - concurrent-aware lock-less design;
 * - considered border cases where simple scanning performs better and is
 *   employed appropriatelly;
 * - code is covered with unit tests and benchmarking snippets.
 *
 * However, hitmap has some limitations:
 * 
 * - O( log32( n ) )/O( log64( n ) ) for bit setting/unsetting as well;
 * - constant factors comparing with simple straightforward implementation
 * 
 *   You may see performance drop comparing with simple scan if your map is
 *   populated densely and uniformly with kind of bits you look for (set/unset).
 *   Although code has short paths for particular situations, short paths can't
 *   be introduced for all cases.
 * 
 * - concurrent writers aren't supported;
 *
 *   Supports "multiple readers/the sole writer" scheme.
 * 
 * - CPU memory barriers should be injected by caller;
 * - size of map can be only power of 2.
 *
 * To make long story short, hitmap is worth employing if calling code:
 * - does map look-ups frequently and map is sparsed in the most cases;
 * - needs strong time complexity guarantee for all operations.
 *
 * As it was mentioned above, hitmap implementation has short paths for
 * particular situations. For example, if your map is little (about 8 machine
 * words) then hitmap algorithm won't be employed at all. So, you can use this
 * data structure in the vast majority of cases.
 */

/**
 * @page algo Algorithm
 *
 * Here is graphical representation of map which consists of 256 bit positions:
 * 
 * ![2-level hitmap]( ../resources/2level_hitmap.png )
 *
 * The core idea of hitmap can be noticed just from this picture. The idea is
 * to use several bit vectors with different levels of detalization regarding
 * positions they represent. There is 0-level vector which is 1:1
 * representation. It means that each bit from this vector corresponds just
 * one position in view for API user.
 *
 * The next level (first, for example) has a bit more complicated structure:
 * - Firstly, each bit at this level represents more than 1 position of user
 *   view. Pictured 1 level has 1:8 accordance with user view.
 * - Secondly, the levels starting with level 1 have complex structure: each
 *   positive word has corresponding negative word. Hence real size of a level
 *   is two times bigger than its logical size.
 *
 * Let's consider some facts separately.
 *
 * Each bit in positive word of level which number is > 0 is set when at least
 * one of the corresponding bits in positive word at a lower level is set.
 * To clarify terminology, it is better to mention that all words at level 0
 * are positive words.
 *
 * Each bit in negative word of level which number is > 0 is set when at least
 * one of the corresponding bits in negative word at a lower level is set.
 * Negativity means that a negative word reflects presence of unset bits at
 * level 0.
 *
 * Positive and negative words altrenate with each other in vector. It means
 * that, for example, word #0 at level 1 is positive word corresponding to
 * particular word range at level 0, but word #1 at level 1 is negative word
 * for the same range at level 0.
 *
 * Further, it's needed to mention that a bit at higher level (both positive and
 * its doppelganger) may represent up to 1 word at a lower layer. If requested
 * size of map (which is power of 2 always) is such that size in words of
 * particular lower level is greater than word size in bits then additional
 * positive and negative word will be introduced at the level which is next to
 * given level.
 *
 * Such hierarchical structure provides logarithmic grows of time complexity
 * for look-ups. By look-up, they mean process of finding next set/unset bit
 * beginning with particular position. For example, if algorithm haven't found
 * set/unset bit at the word where current position is, it moves to the next
 * level and tries to find something there at corresponding negative/positive
 * word. This process lasts until algorithm approaches the highest level of map
 * or catches desirable bit state at a lower level. From this point, it's needed
 * to clarify position in found region. From this point, algorithm begin to step
 * down the hierarchy to clarify exact index of set/unset position with help
 * of lower level and with evidence provided by higher levels.
 *
 * It's obvious that hierarchical structure of map complicates process of
 * setting/unsetting bit in the map. To update bit at particular index,
 * algorithm have to check bit's neighbors and modify bits at higher levels
 * according to the state of the bit and its neighbors. This operation have to
 * be repeated for each level up to the highest one.
 *
 * It's needed to be mentioned that during the process of index clarification
 * discover process (which goes down the hierarchy) could potentially meet
 * concurrent modification process (which goes up the hierarchy). If the state
 * of things such that there is no bit with desirable state at current region
 * then discovery process switches to going up the hierarchy starting with
 * current yet unclarified position. Discovering up moves forward to bigger
 * indexes. Such property guarantees that algorithm will return some result in
 * limited amount of time under high contention of read/write operations.
 *
 * To provide another bit of terminology, it's needed to be clarified that
 * process of going up is called "bob up" and the process of going down (or
 * "clarification") is called "sink".
 * 
 * As an optimization step, level #1 appears when requested map capacity is greater
 * than 8 words.
 *
 * ![1-level hitmap]( ../resources/1level_hitmap.png )
 *
 * Other levels appears when corresponding previous level size in
 * words is greater than 2. Mentioning other short paths and optimizations, map
 * discover process switches to simple linear scanning if the distance from
 * the current position to the end of first level is less than 8 words (this
 * magic number comes from empirical investigation).
 */

#ifndef LIBHITMAP
#define LIBHITMAP

#include <atomic_ops.h>
#include <limits.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of bits in minimal map.
 */
#define HITMAP_INITIAL_SIZE ( sizeof( AO_t ) * 8 )

/**
 * @internal
 *
 * Logarithm base 2 from the number which is length of machine word in bits.
 */
#if ( ULONG_MAX != 4294967295UL )
	#define _WORD_POW ( 6 )
#else
	#define _WORD_POW ( 5 )
#endif

/**
 * @internal
 *
 * Logarithm base 2 from the threshold of switching between siple linear
 * algorithm and hitmap algorithm.
 */
#define _DUMMY_THRESHOLD_POW ( _WORD_POW + 3 )
/**
 * @internal
 *
 * Threshold of switching between siple linear algorithm and hitmap algorithm.
 */
#define _DUMMY_THRESHOLD ( 1ul << _DUMMY_THRESHOLD_POW )

/**
 * Hitmap data structure.
 */
typedef struct {
	int len_pow; /*!< Logarithm base 2 from the logic capacity of map.*/
	AO_t *bits_border; /*!< Pointer to the word after the last word of bits[]
							array. Needed for optimization purposes in
							hitmap_has.*/
	AO_t bits[]; /*!< Map data.*/
} map_t;

/**
 * Operation selector type.
 * 
 * This enumeration is used in almost all API calls as operation selector.
 */
enum hitmap_mark {
	BIT_UNSET = 1, /*!< Search for unset position or make position unset. */
	BIT_SET = 0 /*!< Search for set position or make position set. */
};

/**
 * @internal
 *
 * Calculates words num in real hitmap.
 *
 * Calculates actual size in words of map_t::bits array in case if it will be
 * real hierarchical map.
 * @param len_pow binary logarithm from requested number of bits
 * @return size in words for map_t::bits array for passed number of bits
 * @see hitmap_calc_sz
 */
extern size_t _hitmap_calc_elements_num( int len_pow );

/**
 * @internal
 *
 * Calculates particular power of 2.
 * 
 * Simple ancillary function servicing frequent need to calculate power of 2.
 * @param pow power which 2 should be raised
 * @return power of 2 number
 */
static inline unsigned long int _pow2( int pow ) {
	return ( 1ul << pow );
}

/**
 * Calculates size of hitmap in bytes.
 * 
 * Calculates actual size of map_t structure in bytes from given
 * binary logarithm of requested capacity. The fact follows from the description
 * is that capacity can be only power of two.
 * @param len_pow binary logarithm from requested number of bits
 * @return size in bytes of map_t structure contains defined capacity; may be
 *         passed directly to malloc/alloca as argument for allocating
 * @see hitmap_init
 */
static inline size_t hitmap_calc_sz( int len_pow ) {
	return ( sizeof( map_t ) +
		(
			( len_pow <= _DUMMY_THRESHOLD_POW ) ?
				_pow2( ( len_pow > _WORD_POW ) ? ( len_pow - _WORD_POW ) : 0 ) :
				_hitmap_calc_elements_num( len_pow )
		) * sizeof( AO_t )
	);
}

/**
 * @internal
 * 
 * Initializes allocated map as hitmap.
 * 
 * Initializes passed structure as hierarchical map according to requested
 * capacity.
 * @param map allocated map structure
 * @param len_pow binary logarithm derived from requested number of bits
 * @see hitmap_calc_sz
 * @see _hitmap_calc_elements_num
 */
extern void _hitmap_init( map_t *map, int len_pow );

/**
 * Initializes allocated map for further use.
 * 
 * Allocation of bitmap should be handled by user. Library provides function for
 * appropriate initialization only. Firstly, size of map should be calculated
 * with help of hitmap_calc_sz and passed to allocator. Then, allocated map_t
 * should be initialized with help of hitmap_init.
 * @param map allocated map structure
 * @param len_pow binary logarithm derived from requested number of bits
 * @see hitmap_calc_sz
 */
static inline void hitmap_init( map_t *map, int len_pow ) {
	// capacity of map can't be less than 32/64 positions
	if( len_pow < _WORD_POW )
		len_pow = _WORD_POW;

	map->len_pow = len_pow;

	if ( len_pow <= _DUMMY_THRESHOLD_POW ) {
		// more optimal to use linear simple algorithm
		map->bits_border = NULL;
		memset( map->bits,
			0,
			sizeof( AO_t ) * _pow2( len_pow - _WORD_POW )
		);
	} else
		_hitmap_init( map, len_pow );
}

/**
 * @internal
 * 
 * Sets or unsets bit with hitmap algorithm.
 * 
 * According to passed is_put operation selector, sets or unsets bit in map
 * with specified index according to hitmap algorithm ( O( log32(n) ) /
 * O( log64(n) ) ).
 * @param map initialized map structure
 * @param idx linear index of bit in map
 * @param is_put operation selector (set/unset)
 * @see hitmap_change_for
 */
extern void _hitmap_change_for( map_t *map,
	size_t idx,
	enum hitmap_mark is_put
);

/**
 * @internal
 * 
 * Sets or unsets bit with simple algorithm.
 * 
 * According to passed is_put operation selector, sets or unsets bit in map
 * with specified index with simple set in bit vector ( O( 1 ) ).
 * @param map initialized map structure
 * @param idx linear index of bit in map
 * @param is_put operation selector (set/unset)
 * @see hitmap_change_for
 */
extern void _dummy_change_for( map_t *map,
	size_t idx,
	enum hitmap_mark is_put
);

/**
 * Sets or unsets bit with specified index.
 * 
 * According to passed is_put operation selector, sets or unsets bit in map
 * with specified index.
 * @param map initialized map structure
 * @param idx linear index of bit in map
 * @param is_put operation selector (set/unset)
 * @see hitmap_discover
 */
static inline void hitmap_change_for( map_t *map,
	size_t idx,
	enum hitmap_mark is_put
) {
	assert( map != NULL );
	assert( idx < _pow2( map->len_pow ) );

	if ( map->len_pow <= _DUMMY_THRESHOLD_POW )
		_dummy_change_for( map, idx, is_put );
	else
		_hitmap_change_for( map, idx, is_put );
};

/**
 * @internal
 * 
 * Set/unset bit look-up with hitmap algorithm.
 * 
 * According to passed which operation selector, looks for set/unset bit in map
 * starting from specified index inclusively with help of "bob up"-"sink"
 * algorithm ( O( log32(n) ) / O( log64(n) ) ).
 * @param map initialized map structure
 * @param start_idx starting point of search
 * @param which operation selector (look for set/unset bit)
 * @return index of first occurence of set/unset bit
 * @see hitmap_discover
 */
extern size_t _hitmap_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
);

/**
 * @internal
 * 
 * Set/unset bit linear search.
 * 
 * According to passed which operation selector, searches linearly for *which*
 * flavour of bit ( O(n) ).
 * @param map initialized map structure
 * @param start_idx starting point of search
 * @param which operation selector (look for set/unset bit)
 * @return index of first occurence of set/unset bit
 * @see hitmap_discover
 */
extern size_t _dummy_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
);

/**
 * Looks for set/unset bit starting from specified index.
 * 
 * According to passed which operation selector, looks for set/unset bit in map
 * starting from specified index inclusevly.
 * @param map initialized map structure
 * @param start_idx starting point of search
 * @param which operation selector (look for set/unset bit)
 * @return index of first occurence of set/unset bit
 * @see hitmap_change_for
 * @see hitmap_has
 */
inline static size_t hitmap_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	assert( map != NULL );
	assert( start_idx < _pow2( map->len_pow ) );

	// if distance between current position and the end of map is less than
	// threshold then simple linear scan is employed
	return ( ( _pow2( map->len_pow ) - start_idx ) <= _DUMMY_THRESHOLD ) ?
		_dummy_discover( map, start_idx, which ) :
		_hitmap_discover( map, start_idx, which );
}

/**
 * @internal
 * 
 * Tests specified map for set/unset bits with linear scan.
 * 
 * According to passed which operation selector, tests for presence of set/unset
 * bits in specified map with simple linear scan starting from beginning
 * ( O( n ) ).
 * @param map initialized map structure
 * @param which operation selector (look for set/unset bit)
 * @return != 0 - if map has specified kind of bits; == 0 - otherwise
 * @see hitmap_has
 */
extern int _dummy_has( map_t *map, enum hitmap_mark which );

/**
 * @internal
 * 
 * Tests specified map for set/unset bits with hitmap trick.
 * 
 * According to passed which operation selector, tests for presence of set/unset
 * bits in specified map with O( 1 ) checking of the highest level of hitmap.
 * @param map initialized map structure
 * @param which operation selector (look for set/unset bit)
 * @return != 0 - if map has specified kind of bits; == 0 - otherwise
 * @see hitmap_has
 */
extern int _hitmap_has( map_t *map, enum hitmap_mark which );

/**
 * Tests if specified map has set/unset positions.
 * 
 * According to passed which operation selector, tests for presence of set/unset
 * bits in specified map.
 * @param map initialized map structure
 * @param which operation selector (look for set/unset bit)
 * @return != 0 - if map has specified kind of bits; == 0 - otherwise
 * @see hitmap_change_for
 * @see hitmap_discover
 */
static inline int hitmap_has( map_t *map, enum hitmap_mark which ) {
	assert( map != NULL );

	return ( map->len_pow <= _DUMMY_THRESHOLD_POW ) ?
		_dummy_has( map, which ) :
		_hitmap_has( map, which );
}

#ifdef __cplusplus
}
#endif

#endif
