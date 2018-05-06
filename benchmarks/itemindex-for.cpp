#include <iostream>
#include <iomanip>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateFoR.h>
#include <sserialize/stats/TimeMeasuerer.h>

using namespace sserialize;

/*
 TODO: compare with https://github.com/lemire/FastPFor

 compare this to https://pastebin.com/ugGnk00p, blog post: https://lemire.me/blog/2012/03/06/how-fast-is-bit-packing/
 Timings on Intel(R) Core(TM) i7-4700MQ CPU @ 2.40GHz
 gcc (Debian 6.3.0-18+deb9u1) 6.3.0 20170516
 clang version 6.0.0-1~bpo9+1 (tags/RELEASE_600/final)

This with gcc:
g++-6 -std=c++17 -g -O3 -march=native -flto -fno-fat-lto-objects -frounding-math
bits    packtime        unpacktime
1       342     118
2       383     119
3       478     118
4       472     120
5       628     121
6       679     119
7       770     119
8       684     118
9       971     120
10      876     118
11      972     120
12      914     124
13      898     118
14      907     119
15      971     119
16      898     118
17      1035    119
18      924     118
19      1003    120
20      934     118
21      933     119
22      943     118
23      1019    119
24      949     118
25      1139    120
26      962     119
27      1065    121
28      989     119
29      986     122
30      990     119
31      1054    121


The one from Lemire:
g++ -O3 -march=native -o bitpacking bitpacking.cpp
bits    packtime        unpacktime
1       165     190
2       161     196
3       165     195
4       156     153
5       171     200
6       171     202
7       175     200
8       145     159
9       183     206
10      186     208
11      189     211
12      182     211
13      196     211
14      195     215
15      201     215
16      151     178
17      207     220
18      212     221
19      216     223
20      212     225
21      220     227
22      227     228
23      230     277
24      215     231
25      235     235
26      236     235
27      247     238
28      240     240
29      250     240
30      250     244
31      257     245
32      215     216

clang++-6.0 -O3 -march=native -o bitpacking_clang bitpacking.cpp
bits    packtime        unpacktime
1       139     160
2       136     171
3       144     181
4       120     180
5       165     187
6       147     182
7       165     189
8       134     207
9       208     212
10      172     214
11      200     212
12      181     211
13      200     216
14      192     219
15      203     219
16      160     221
17      315     223
18      296     223
19      306     227
20      272     226
21      298     232
22      291     233
23      316     231
24      290     234
25      434     226
26      310     215
27      343     228
28      296     231
29      293     227
30      255     226
31      226     225
32      217     220

*/

int main() {
	uint64_t blockSize = 1 << 25;
	uint32_t runs = 16;
	
	
	sserialize::TimeMeasurer tmEncode;
	sserialize::TimeMeasurer tmDecode;
	UByteArrayAdapter dest(0, MM_PROGRAM_MEMORY);
	std::vector<uint32_t> src(blockSize);
	detail::ItemIndexImpl::FoRBlock block(dest, 0, 0, 1);
	
	std::cout << "bits\tpacktime[ms]\tunpacktime[ms]\tunpack [M/s]" << std::endl;
	for(uint32_t bits(1); bits < 32; ++bits) {
		dest.resetPtrs();
		
		uint32_t mask = sserialize::createMask(bits);
		for(uint32_t i(0); i < blockSize; ++i) {
			src[i] = i & mask;
		}
		
		tmEncode.begin();
		detail::ItemIndexImpl::FoRCreator::encodeBlock(dest, src.begin(), src.end(), bits);
		tmEncode.end();
		
		if (dest.tellPutPtr() != (bits*blockSize/8 + uint64_t((bits*blockSize%8)>0))) {
			std::cerr << "Compressed size wrong: " << dest.tellGetPtr() << std::endl;
			return -1;
		}
		
		block.update(dest, 0, blockSize, bits);
		
		for(uint32_t i(0), prev(0); i < blockSize; ++i) {
			prev += src[i];
			if (block.at(i) != prev) {
				std::cerr << "ERROR: block.at(" << i << ")=" << block.at(i) << "!=" << prev << std::endl;
				return -1;
			}
		}
		
		tmDecode.begin();
		for(uint32_t i(0); i < runs; ++i) {
			block.update(dest, 0, blockSize, bits);
		}
		tmDecode.end();
		std::cout << std::setprecision(4);
		std::cout << bits << '\t' << tmEncode.elapsedMilliSeconds() << '\t' << tmDecode.elapsedMilliSeconds()/runs << '\t';
		std::cout << ((blockSize*runs)/(double(tmDecode.elapsedUseconds())/1000000))/1000000 << std::endl;
	}
}
