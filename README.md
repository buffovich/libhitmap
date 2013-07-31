libhitmap
=========

Hierarchical bitmap (or bit-trie).

Hierarchical bitmap (bit vector) structure (in short - "hitmap") provides
an user with:

 - compact view of boolean vector;

   You need only 40 (32-bit)/48 (64-bit) bytes for 256 booleans and
   144 (on 32-bit)/ 160 (on 64-bit) for 1024 booleans representation.
 
 - O( log32( n ) )/O( log64( n ) ) complexity of looking for the next
   set/unset bit index;
 - O( 1 ) complexity of defining if map has set/unset bits;
 - concurrent-aware lock-less design;
 - considered border cases where simple scanning performs better and is
   employed appropriatelly;
 - code is covered with unit tests and benchmarking snippets.

 However, hitmap has some limitations:
 
 - O( log32( n ) )/O( log64( n ) ) for bit setting/unsetting as well;
 - constant factors comparing with simple straightforward implementation
 
   You may see performance drop comparing with simple scan if your map is
   populated densely and uniformly with kind of bits you look for (set/unset).
   Although code has short paths for particular situations, short paths can't
   be introduced for all cases.
 
 - concurrent writers aren't supported;

   Supports "multiple readers/the sole writer" scheme.
 
 - CPU memory barriers should be injected by caller.

 To make long story short, hitmap is worth employing if calling code:
 - does map look-ups frequently and map is sparsed in the most cases;
 - needs strong time complexity guarantee for all operations.

As it was mentioned above, hitmap implementation has short paths for
particular situations. For example, if your map is little (about 8 machine
words) then hitmap algorithm won't be employed at all. So, you can use this
data structure in the vast majority of cases.

Installation
------------

Just to build the library you need:
 - GNU make;
 - GCC toolset. 

Firtsly, check-out/clone repository into build environment:

`> git clone https://github.com/buffovich/libhitmap.git .`

Secondly, cd to libhitmap:

`> cd ./libhitmap`

Thirdly, run make:

`> make`

That's it. You will find dynamic and static version of compiled library at the
folder you are at now.

If you need debug version of library, please run following:

`> make build DEBUG=3`

To run unit-tests, you need installed [check](http://check.sourceforge.net/)
library:

`> make test`

To see the coverage report, you will need
[LCOV](http://ltp.sourceforge.net/coverage/lcov.php) tool installed as well as
Just mentioned check framework. Just before coverage run, you will have to
rebuild library if it has been built already:

`> make clean`

`> make coverage`

(Linux only) To see run of unit-test with memory checking, you will need
Valgrind in addition to "check". Just run following:

`> make memcheck`

You will see Memcheck report at tests dir.

(Linux only) To benchmark code, you will need Valgrind also. Run following:

`> make memcheck`

Reports for all data combinations will be placed at ./benchmarks directory.
To get the idea about what data represents, please check benchmarking code.
