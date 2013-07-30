#define _GNU_SOURCE

#include <hitmap.h>

#include <atomic_ops.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#if defined( __GNUC__ ) && !defined( __INTEL_COMPILER )
	#define ffsl __builtin_ffsl
#endif

#define _BIT_NUMBER_MASK ( HITMAP_INITIAL_SIZE - 1 )

inline static size_t __ffsl_after( AO_t mword, size_t idx ) {
	if( mword >>= idx )
		return ( idx + ffsl( mword ) );
	else
		return 0;

	return 0;
}

inline static size_t __ffcl_after( AO_t mword, size_t idx ) {
	return __ffsl_after( ~ mword, idx );
}

size_t _dummy_discover( map_t *map,
	size_t start_idx,
	enum hitmap_mark which
) {
	size_t wcapacity = 1ul << ( map->len_pow - _WORD_POW ),
		bitn = 0,
		cyc = start_idx >> _WORD_POW;

	switch( which ) {
		case BIT_SET:
			bitn = __ffsl_after(
				AO_load( map->bits + cyc ),
				start_idx & _BIT_NUMBER_MASK
			);
			if( bitn != 0 )
				return ( cyc << _WORD_POW ) | ( bitn - 1 );
			++cyc;
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
	len_pow -= _WORD_POW;
	size_t sz = 1ul << len_pow;
	while( len_pow > _WORD_POW ) {
		len_pow -= _WORD_POW;
		sz += 1ul << ( len_pow + 1 );
	}

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
		for( AO_t *cur = map->bits + ( 1ul << len_pow ) + 1,
				*limit = map->bits + el_num;
			cur < limit;
			cur += 2
		)
			*cur = SIZE_MAX;
}

static inline AO_t __group_mask_for_bit( int len_pow, size_t idx ) {
	if( len_pow >= _WORD_POW )
		return SIZE_MAX;

	return ( ( 1ul << ( 1ul << len_pow ) ) - 1 ) <<
		( idx & ( ~ ( ( 1ul << len_pow ) - 1 ) ) & _BIT_NUMBER_MASK );
}

void _dummy_change_for( map_t *map,
	size_t idx,
	enum hitmap_mark is_put
) {
	switch( is_put ) {
		case BIT_SET:
			map->bits[ idx >> _WORD_POW ] |= 1ul << ( idx & _BIT_NUMBER_MASK );
			break;
		case BIT_UNSET:
			map->bits[ idx >> _WORD_POW ] &=
				~ ( 1ul << ( idx & _BIT_NUMBER_MASK ) );
			break;
	}
}

void _hitmap_change_for( map_t *map, size_t idx, enum hitmap_mark is_put ) {
	size_t widx = idx >> _WORD_POW;
	int len_pow = map->len_pow - _WORD_POW;
	switch( is_put ) {
		case BIT_SET:
			map->bits[ widx ] |= 1ul << ( idx & _BIT_NUMBER_MASK );
			break;
		case BIT_UNSET:
			map->bits[ widx ] &= ~ ( 1ul << ( idx & _BIT_NUMBER_MASK ) );
			break;
	}

	if( len_pow < 2 )
		return;

	AO_t mask = __group_mask_for_bit( len_pow, idx );
	int is_full = ( is_put == BIT_SET ) ?
		( ( map->bits[ widx ] & mask ) == mask ) :
		( ( map->bits[ widx ] & mask ) == 0 );
	AO_t bit = 0;
	AO_t *level_border = map->bits + ( 1ul << len_pow );
	while( len_pow > _WORD_POW ) {
		len_pow -= _WORD_POW;
		idx >>= _WORD_POW;
		widx >>= _WORD_POW;

		bit = 1ul << ( idx & _BIT_NUMBER_MASK );
		level_border[ ( widx << 1 ) + is_put ] |= bit;

		if( is_full )
			is_full = (
				(
					( level_border[ ( widx << 1 ) + 1 - is_put ] &= ~bit ) &
					__group_mask_for_bit( len_pow, idx )
				)
			) == 0;

		level_border += ( 1ul << len_pow ) << 1;
	}

	if( len_pow >= 2 ) {
		idx >>= len_pow;

		bit = 1ul << ( idx & _BIT_NUMBER_MASK );
		level_border[ is_put ] |= bit;

		if( is_full )
			level_border[ 1 - is_put ] &= ~bit;
	}
}

inline static size_t __get_bit_number( AO_t *level_border,
	size_t idx,
	enum hitmap_mark which
) {
	return __ffsl_after(
		AO_load( level_border + ( ( idx >> _WORD_POW ) << 1 ) + which ),
		idx & _BIT_NUMBER_MASK
	);
}

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
			bitn = __get_bit_number( level_border, idx, which );

			if( bitn != 0 )
				break;

			idx = ( idx & ( ~ _BIT_NUMBER_MASK ) ) + HITMAP_INITIAL_SIZE;

			if( idx >= ( 1ul << cur_pow )  )
				return 0;

			if( cur_pow > 2 * _WORD_POW ) {
				cur_pow -= _WORD_POW;
				level_border += ( 1ul << ( cur_pow + 1 ) );
				idx >>= _WORD_POW;
			} else {
				if( cur_pow >= ( 2 + _WORD_POW ) ) {
					cur_pow -= _WORD_POW;
					level_border += ( 1ul << ( cur_pow + 1 ) );
					idx >>= cur_pow;
				}

				bitn = __get_bit_number( level_border, idx, which );
				break;
			}
		}
	else
		bitn = __get_bit_number( level_border, idx, which );

	if( bitn != 0 ) {
		*idxp =( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 );
		*cur_powp = cur_pow;
		*level_borderp = level_border;
		return 1;
	}

	return 0;
}

static int __map_sink( AO_t **level_borderp,
	int len_pow,
	int *cur_powp,
	size_t *idxp,
	enum hitmap_mark which
) {
	int cur_pow = *cur_powp;

	AO_t *level_border = *level_borderp - ( 1ul << ( cur_pow + 1 ) );
	size_t idx = *idxp << ( ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow );
	cur_pow += _WORD_POW;

	size_t bitn = 0;
	while( cur_pow < len_pow ) {
		bitn = __get_bit_number( level_border, idx, which );

		if( bitn == 0 ) {
			*idxp = idx;
			*level_borderp = level_border;
			*cur_powp = cur_pow;
			return 0;
		}

		idx = ( ( idx & ( ~ _BIT_NUMBER_MASK ) ) | ( bitn - 1 ) ) << _WORD_POW;
		level_border -= ( 1ul << ( cur_pow + 1 ) );
		cur_pow += _WORD_POW;
	}

	*idxp = idx;

	return 1;
}

static inline AO_t __load_word( map_t *map , size_t idx ) {
	return AO_load( map->bits + ( idx >> _WORD_POW ) );
}

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
	size_t new_idx = __scan_0_level( map, start_idx, which );
	if( new_idx != SIZE_MAX )
		return new_idx;

	start_idx += HITMAP_INITIAL_SIZE;
	if( start_idx >= ( 1ul << map->len_pow )  )
		return SIZE_MAX;

	int cur_pow = map->len_pow - _WORD_POW;
	AO_t *level_border = map->bits + ( 1ul << cur_pow );
	start_idx >>= ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow;
	int initial_cur_pow = cur_pow;
	while( 1 ) {
		do{
			if( ! __map_bob_up(
				&level_border, &cur_pow, &start_idx, which
			) )
				return SIZE_MAX;

			if( ( cur_pow + _WORD_POW ) >= map->len_pow ) {
				start_idx <<= ( cur_pow < _WORD_POW ) ? cur_pow : _WORD_POW;
				break;
			}
		} while(
			! __map_sink(
				&level_border, map->len_pow, &cur_pow, &start_idx, which
			)
		);

		if(
			( new_idx = __scan_0_level( map, start_idx, which ) ) !=
			SIZE_MAX
		)
			return new_idx;

		start_idx += HITMAP_INITIAL_SIZE;
		if( start_idx >= ( 1ul << map->len_pow )  )
			return SIZE_MAX;

		cur_pow = initial_cur_pow;
		level_border = map->bits + ( 1ul << cur_pow );
		start_idx >>= ( cur_pow > _WORD_POW ) ? _WORD_POW : cur_pow;
	}

	return SIZE_MAX;
}

int _dummy_has( map_t *map, enum hitmap_mark which ) {
	size_t limit = 1ul << ( map->len_pow - _WORD_POW );
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
	size_t shift = ( ( map->len_pow % _WORD_POW ) == 1 ) ? 2 : 1;

	if( AO_load( map->bits_border - ( shift * 2 ) + which ) != 0 )
		return 1;
	else {
		if( shift > 1 )
			return ( AO_load( map->bits_border - 2 + which ) != 0 );
	}

	return 0;
}
