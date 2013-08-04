/**
 * @internal
 *
 * Switch on extended GLibC functionality (ffsl as an example).
 */
#define _GNU_SOURCE

#include <hitmap.h>

#include <atomic_ops.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/**
 * @internal
 *
 * Direct mapping ffsl routine to GCC intrinsic.
 *
 * ffsl is about "find first set bit in long int".
 */
#if defined( __GNUC__ ) && !defined( __INTEL_COMPILER )
	#define ffsl __builtin_ffsl
#endif

/**
 * @internal
 *
 * Mask for calculating bit number from position index.
 *
 * Applying this mask on index, you will get bit number in the word. For 32-bit
 * platform its 0b11111.
 */
#define _BIT_NUMBER_MASK ( HITMAP_INITIAL_SIZE - 1 )

/**
 * @internal
 *
 * ffsl which scans word after idx bit.
 *
 * For example, if you need to scan a word starting from particular position in
 * word skipping idx bits situated before position.
 * @param mword word to scan
 * @param idx index of start position in word
 * @return index of set bit in word after mentioned position; indexing starts
 *         with 1
 * @see __ffcl_after
 */
inline static size_t __ffsl_after( AO_t mword, size_t idx ) {
	if( mword >>= idx )
		return ( idx + ffsl( mword ) );
	else
		return 0;

	return 0;
}

/**
 * @internal
 *
 * Like ffsl but looks for unset bit.
 *
 * See __ffsl_after.
 * @param mword word to scan
 * @param idx index of start position in word
 * @return index of unset bit in word after mentioned position; indexing starts
 *         with 1
 * @see __ffsl_after
 */
inline static size_t __ffcl_after( AO_t mword, size_t idx ) {
	return __ffsl_after( ~ mword, idx );
}

size_t _dummy_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	
	size_t wcapacity = _pow2( map->len_pow - _WORD_POW ),
		bitn = 0,
		/* shifting gives us index of word in level 0*/
		cyc = start_idx >> _WORD_POW;

	switch( which ) {
		case BIT_SET:
			/* short path */
			bitn = __ffsl_after(
				AO_load( map->bits + cyc ),
				start_idx & _BIT_NUMBER_MASK
			);
			if( bitn != 0 )
				return ( cyc << _WORD_POW ) | ( bitn - 1 );
			++cyc;
			/* long path */
			for( ; cyc < wcapacity; ++cyc ) {
				bitn = ( size_t ) ffsl( AO_load( map->bits + cyc ) );
				if( bitn != 0 )
					return ( cyc << _WORD_POW ) | ( bitn - 1 );
			}
			break;
		case BIT_UNSET:
			bitn = __ffcl_after(
				AO_load( map->bits + cyc ),
				start_idx & _BIT_NUMBER_MASK
			);
			if( bitn != 0 )
				return ( cyc << _WORD_POW ) | ( bitn - 1 );
			++cyc;
			for( ; cyc < wcapacity; ++cyc ) {
				bitn = ( size_t ) ffsl( ~ AO_load( map->bits + cyc ) );
				if( bitn != 0 )
					return ( cyc << _WORD_POW ) | ( bitn - 1 );
			}
			break;
	}

	return SIZE_MAX;
}

size_t _hitmap_calc_elements_num( int len_pow ) {
	len_pow -= _WORD_POW; /* to find out level 0 size in words */
	size_t sz = _pow2( len_pow );
	while( len_pow > _WORD_POW ) {
		/* recall that bit at a higher level can represent
		 * a region up to 32/64 bits at a lower level */
		len_pow -= _WORD_POW;
		/* +1 stands for negative word after each position word on levels > 0 */
		sz += _pow2( len_pow + 1 );
	}

	/* if there are 4 or more pos-neg pairs at the highest level
	 * then we need additional level; 2 is threshold
	 * (for optimization purposes) */
	if( len_pow >= 2 )
		sz += 2;

	return sz;
}

void _hitmap_init( map_t *map, int len_pow ) {
	size_t el_num = _hitmap_calc_elements_num( len_pow );
	map->bits_border = map->bits + el_num; 
	memset( map->bits, 0, el_num * sizeof( AO_t ) );

	len_pow -= _WORD_POW;
	if( len_pow >= 2 )
		for( AO_t *cur = map->bits + _pow2( len_pow ) + 1,
				*limit = map->bits + el_num;
			cur < limit;
			cur += 2
		)
			*cur = SIZE_MAX;
}

/**
 * @internal
 *
 * Calculates neighborhood mask.
 *
 * Calculates mask which represents all bits at a lower level which belong to
 * particular one bit at a higher level. For example if lower level has 128
 * positions and idx is 0,1,2, or 3 then neighborhood mask will be 0b1111
 * @param len_pow logarithm base 2 from byte length of the current level
 * @param idx bit index in the map which neighborhood mask should be calculated
 * @return neighborhood mask
 */
static inline AO_t __group_mask_for_bit( int len_pow, size_t idx ) {
	/* short path (recall maximum number of bits
	 * can be represented by bit at higher level) */
	if( len_pow >= _WORD_POW )
		return SIZE_MAX;

	/* Let's consider each operationseparately:
	 * lph = 2 ^ len_pow - number of lower bits per higher bit;
	 * mask = ( 2 ^ ( lph ) - 1 ) - unshifted neighbor mask
	 *   (we will have to place it to the right region of word with shift)
	 * local_idx = idx & _BIT_NUMBER_MASK - index of bit in word which we are
	 *   going to calculate neighbours mask
	 * ~ ( lph - 1 ) - mask for common part on index number for all neighbour
	 *   bits group
	 */
	return ( _pow2( _pow2( len_pow ) ) - 1 ) <<
		( idx & _BIT_NUMBER_MASK & ( ~ ( _pow2( len_pow ) - 1 ) ) );
}

void _dummy_change_for( map_t *map,
	size_t idx,
	enum hitmap_mark is_put
) {
	switch( is_put ) {
		case BIT_SET:
			map->bits[ idx >> _WORD_POW ] |= _pow2( idx & _BIT_NUMBER_MASK );
			break;
		case BIT_UNSET:
			map->bits[ idx >> _WORD_POW ] &= ~ _pow2( idx & _BIT_NUMBER_MASK );
			break;
	}
}

void _hitmap_change_for( map_t *map, size_t idx, enum hitmap_mark is_put ) {
	/* modifying level 0 */
	_dummy_change_for( map, idx, is_put );

	size_t widx = idx >> _WORD_POW;
	int len_pow = map->len_pow - _WORD_POW;

	/* defining if there are no unset/set bits left
	 * its needed for modifying negative/positive mask
	 * depending on is_put operation selector:
	 *   - if all group neighbours are set then corresponding bit at negative
	 *     mask of higher level should be unset
	 *   - if all group neighbours are unset then corresponding bit at positive
	 *     mask of higher level should be unset
	 */
	AO_t mask = __group_mask_for_bit( len_pow, idx );
	int is_full = ( is_put == BIT_SET ) ?
		( ( map->bits[ widx ] & mask ) == mask ) :
		( ( map->bits[ widx ] & mask ) == 0 );
	/* ******************************************* */

	AO_t bit = 0;
	AO_t *level_border = map->bits + _pow2( len_pow );
	while( len_pow > _WORD_POW ) {
		len_pow -= _WORD_POW;
		idx >>= _WORD_POW;
		widx >>= _WORD_POW;

		bit = _pow2( idx & _BIT_NUMBER_MASK );
		level_border[ ( widx << 1 ) + is_put ] |= bit;

		/* If the group on the previous level are full (see comments for the
		 * first occurence of is_full) then it's a sign that the group on the
		 * current level is full as well.
		 * 
		 * The same opration as before but with taking in account the structure
		 * of levels > 0.
		 */
		if( is_full )
			is_full = (
					( level_border[ ( widx << 1 ) + 1 - is_put ] &= ~bit ) &
					__group_mask_for_bit( len_pow, idx )
				) == 0;

		level_border += _pow2( len_pow + 1 );
	}

	if( len_pow >= 2 ) {
		/* we have extra level because of threshold
		 * number of words at the last level */
		bit = _pow2( idx >> len_pow );
		level_border[ is_put ] |= bit;

		if( is_full )
			level_border[ 1 - is_put ] &= ~bit;
	}
}

/**
 * @internal
 *
 * Get the next set/unset bit index at the same word.
 * @param level_border starting point of the current level
 * @param idx index of start position for scanning
 * @param which flavour of bit we are looking for
 * @return local bit index in word
 * @see __map_bob_up
 * @see __map_sink
 */
inline static size_t __get_next_bit_index( AO_t *level_border,
	size_t idx,
	enum hitmap_mark which
) {
	return __ffsl_after(
		AO_load( level_border + ( ( idx >> _WORD_POW ) << 1 ) + which ),
		idx & _BIT_NUMBER_MASK
	);
}

/**
 * @internal
 *
 * Stepping up the map hierarachy.
 *
 * First stage of the hitmap discover algorithm. To find something, we look at
 * the map from different heights up to the highest level.
 * @param level_borderp pointer to modifiable variable storing pointer to
 *                      current level of map; serves for passing current level
 *                      between "bob up" and "sink" stages
 * @param cur_powp pointer to modifiable variable storing logarithm base 2 from
 *                 bit capacity of the current level; serves the same purpose
 * @param idxp pointer to modifiable variable storing bit index at the
 *             *level_borderp level; serves the same purpose
 * @param which operation selector
 * @return == 1 if algorithm found desirable kind of bit at particular level
 *         == 0 otherwise (and there is no point to continue discover)
 * @see _hitmap_discover
 */
static int __map_bob_up( AO_t **level_borderp,
	int *cur_powp,
	size_t *idxp,
	enum hitmap_mark which
) {
	int cur_pow = *cur_powp;
	AO_t *level_border = *level_borderp;
	size_t idx = *idxp;

	size_t bitn = 0;
	if( cur_pow > _WORD_POW )
		while( 1 ) {
			bitn = __get_next_bit_index( level_border, idx, which );

			/* if we have found something then we exit bob_up stage */
			if( bitn != 0 )
				break;

			/* no... let's continue stepping up */
			/* since we scanned entire word we should move to the next word */
			/* idx & ( ~ _BIT_NUMBER_MASK ) - global level index of the first
			 *   bit at the current word
			 * + HITMAP_INITIAL_SIZE - move to the next word
			 */
			idx = ( idx & ( ~ _BIT_NUMBER_MASK ) ) + HITMAP_INITIAL_SIZE;

			/* if we have run out of map we exit from discover */
			if( idx >= _pow2( cur_pow )  )
				return 0;

			if( cur_pow > 2 * _WORD_POW ) {
				cur_pow -= _WORD_POW;
				level_border += _pow2( cur_pow + 1 );
				idx >>= _WORD_POW;
			} else {
				if( cur_pow >= ( 2 + _WORD_POW ) ) {
					cur_pow -= _WORD_POW;
					level_border += _pow2( cur_pow + 1 );
					idx >>= cur_pow;
				}

				bitn = __get_next_bit_index( level_border, idx, which );
				break;
			}
		}
	else
		bitn = __get_next_bit_index( level_border, idx, which );

	/* if bob_up has finished successfully we move to sink stage */
	if( bitn != 0 ) {
		*idxp =( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 );
		*cur_powp = cur_pow;
		*level_borderp = level_border;
		return 1;
	}

	return 0;
}

/**
 * @internal
 *
 * Stepping down the map hierarachy.
 *
 * Second stage of the hitmap discover algorithm. We found something at bob_up
 * stage. But it's nothing more than region of the potential treasure. We need
 * to clarify the index in series. For that, algorithm steps down the hierarchy
 * down to the level 1.
 * @param level_borderp pointer to modifiable variable storing pointer to
 *                      current level of map; serves for passing current level
 *                      between "bob up" and "sink" stages
 * @param cur_powp pointer to modifiable variable storing logarithm base 2 from
 *                 bit capacity of the current level; serves the same purpose
 * @param idxp pointer to modifiable variable storing bit index at the
 *             *level_borderp level; serves the same purpose
 * @param which operation selector
 * @return == 1 if algorithm found desirable kind of bit at particular level
 *         == 0 otherwise (we met concurrent modification operation; hence
 *         algorithm will switch back to bob_up stage to find another
 *         opportunity)
 * @see _hitmap_discover
 */
static int __map_sink( AO_t **level_borderp,
	int len_pow,
	int *cur_powp,
	size_t *idxp,
	enum hitmap_mark which
) {
	int cur_pow = *cur_powp;

	AO_t *level_border = *level_borderp - _pow2( cur_pow + 1 );
	size_t idx = *idxp << ( ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow );
	cur_pow += _WORD_POW;

	size_t bitn = 0;
	while( cur_pow < len_pow ) {
		bitn = __get_next_bit_index( level_border, idx, which );

		if( bitn == 0 ) {
			/*  oops! we met concurrent modification operation :
			 * switch back to bob_up stage*/
			 
			/* we need to provide bob_up stage the point we stopped at */
			*idxp = idx;
			*level_borderp = level_border;
			*cur_powp = cur_pow;
			return 0;
		}

		idx = ( ( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 ) ) << _WORD_POW;
		level_border -= _pow2( cur_pow + 1 );
		cur_pow += _WORD_POW;
	}

	*idxp = idx;

	return 1;
}

/**
 * @internal
 *
 * Load word from level 0 with compiler barrier.
 * @param map initialized map structure
 * @param idx bit index which word should be loaded
 * @return loaded word
 * @see __scan_0_level
 */
static inline AO_t __load_word( map_t *map , size_t idx ) {
	return AO_load( map->bits + ( idx >> _WORD_POW ) );
}

/**
 * @internal
 *
 * Scans level 0.
 *
 * Scans level 0 for mentioned flavour of bit. The fastest way to find desirble
 * kind of bit if it is in current word.
 * @param map initialized map structure
 * @param idx global bit index we should start from
 * @param which desirable kind of bit to find
 * @return < SIZE_MAX - bit is found
 *         == SIZE_MAX - current word doesn't contain desirable kind of bit
 */
static inline size_t __scan_0_level( map_t *map,
	size_t idx,
	enum hitmap_mark which
) {
	size_t bitn = ( which == BIT_SET ) ?
		__ffsl_after( __load_word( map, idx ), idx & _BIT_NUMBER_MASK ) :
		__ffcl_after( __load_word( map, idx ), idx & _BIT_NUMBER_MASK );
	idx &= ~ _BIT_NUMBER_MASK;
	if( bitn != 0 )
		return ( idx | ( bitn - 1 ) );

	return SIZE_MAX;
}

size_t _hitmap_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	/* short path */
	size_t new_idx = __scan_0_level( map, start_idx, which );
	if( new_idx != SIZE_MAX )
		return new_idx;

	/* long path */
	start_idx += HITMAP_INITIAL_SIZE;
	/* if we have run out of map we exit with error value */
	if( start_idx >= _pow2( map->len_pow )  )
		return SIZE_MAX;

	int cur_pow = map->len_pow - _WORD_POW;
	AO_t *level_border = map->bits + _pow2( cur_pow );
	start_idx >>= ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow;
	int initial_cur_pow = cur_pow;
	while( 1 ) {
		/* mentioned bob_up-sink cycle
		 * it will spin (number of iteration s will be > 1)
		 * only under high contention */
		do{
			if( ! __map_bob_up(
				&level_border, &cur_pow, &start_idx, which
			) )
				return SIZE_MAX;

			/* if the next level is 0-level the we don't need sinking */
			if( ( cur_pow + _WORD_POW ) >= map->len_pow ) {
				start_idx <<= ( cur_pow < _WORD_POW ) ? cur_pow : _WORD_POW;
				break;
			}
		} while(
			! __map_sink(
				&level_border, map->len_pow, &cur_pow, &start_idx, which
			)
		);

		/* for final clarification we need to scan level 0 */
		if(
			( new_idx = __scan_0_level( map, start_idx, which ) ) !=
			SIZE_MAX
		)
			return new_idx;

		/* we don't find desirable kind of bit at level 0 */
		start_idx += HITMAP_INITIAL_SIZE;
		/* if we have run out of map we exit with error value */
		if( start_idx >= _pow2( map->len_pow )  )
			return SIZE_MAX;

		cur_pow = initial_cur_pow;
		level_border = map->bits + _pow2( cur_pow );
		start_idx >>= ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow;
	}

	return SIZE_MAX;
}

int _dummy_has( map_t *map, enum hitmap_mark which ) {
	size_t limit = _pow2( map->len_pow - _WORD_POW );
	AO_t what = 0;
	switch( which ) {
		case BIT_UNSET:
			what = SIZE_MAX;
			break;
		case BIT_SET:
			what = 0;
			break;
	}

	for( size_t cyc = 0; cyc < limit; ++cyc )
		if( AO_load( map->bits + cyc ) != what )
			return 1;

	return 0;
}

int _hitmap_has( map_t *map, enum hitmap_mark which ) {
	/* consider threshold
	 * ( the last level can contain up to 2 neg-pos pairs ) */
	size_t shift = ( ( map->len_pow % _WORD_POW ) == 1 ) ? 2 : 1;

	/* checking first pair */
	if( AO_load( map->bits_border - ( shift << 1 ) + which ) != 0 )
		return 1;
	else {
		/* checking second pair if it's the case */
		if( shift > 1 )
			return ( AO_load( map->bits_border - 2 + which ) != 0 );
	}

	return 0;
}
