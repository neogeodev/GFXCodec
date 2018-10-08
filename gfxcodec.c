// Furrtek 2018
// NeoGeo graphics codec to/from Genesis format
// To be used with Tile Layer Pro, YY-CHR, Tile Molester...

// TODO: Test CD sprites encode/decode
// TODO: Cart sprites encode/decode

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage() {
	puts("Usage: gfxcodec (-d|e) (-f|s|c) (input file) (output file)");
	puts("       -d|e: decode/encode");
	puts("       -f|s|c: fix/cart sprites/CD sprites");
	exit(EXIT_FAILURE);
}

int main(int argc, char * argv[]) {
	int opt;
	enum { NO_MODE, ENCODE, DECODE } mode = NO_MODE;
	enum { NO_TYPE, FIX, SPR, SPRCD } type = NO_TYPE;
	unsigned long int bitplanes, indexes, mask;
    const unsigned char fix_column_order[4] = { 2, 3, 0, 1 };
    const unsigned char bitplane_masks[4] = { 8, 4, 2, 1 };
    unsigned char byte, color_index, bitplane;
    unsigned char * buffer_in;
    unsigned char * buffer_out;
    FILE * file;
    long int size, i, p, bp;
    
	puts("NeoGeo gfxcodec");
    
    opterr = 0;
    while ((opt = getopt(argc, argv, "defs")) != -1) {
        switch (opt) {
	        case 'd': mode = DECODE; break;
	        case 'e': mode = ENCODE; break;
	        case 'f': type = FIX; break;
	        case 's': type = SPR; break;
	        case 'c': type = SPRCD; break;
	        default:
				print_usage();
        }
    }
    
    if ((mode == NO_MODE) || (type == NO_TYPE) || (optind != 3))
    	print_usage();

	// Load file
    file = fopen(argv[optind], "rb");
    if (!file) {
       printf("Can't open input file.\n");
       return 1;
    }
    
    fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);
	
	buffer_in = malloc(size);		// 4bpp
    if (buffer_in == NULL) {
       printf("Malloc failed.\n");
       return 1;
    }
	
	buffer_out = malloc(size);		// 4bpp
    if (buffer_out == NULL) {
       printf("Malloc failed.\n");
       return 1;
    }
    
    fread(buffer_in, 1, size, file);
    fclose(file);
    
    if (type == FIX) {
	    if (mode == DECODE) {
	    	// Decode fix tiles
		    for (i = 0; i < size; i++) {
		    	// Remove ^ 1 if input isn't byteswapped
				byte = buffer_in[(i & 0xFFFFFFE0UL) + (fix_column_order[i & 3] << 3) + (((i >> 2) & 7) ^ 1)];
				buffer_out[i] = (byte >> 4) | ((byte & 15) << 4);	// Nibble swap
		    }
		} else {
	    	// Encode fix tiles
	    	// Input: AB CD EF GH
	    	//        IJ KL MN OP
	    	// Output: EF MN .. .. .. .. .. .. GH OP .. .. .. .. .. .. AB IJ .. .. .. .. .. .. CD KL .. .. .. .. .. ..
		    for (i = 0; i < size; i++) {
				byte = buffer_in[(i & 0xFFFFFFE0UL) + fix_column_order[(i >> 3) & 3] + ((i & 7) << 2)];
				buffer_out[i ^ 1] = (byte >> 4) | ((byte & 15) << 4);	// Nibble swap
		    }
		}
	} else {
	    if (mode == DECODE) {
	    	// Decode CD sprite tiles
		    for (i = 0; i < size; i+=4) {
				// Bitplanes: 33333333 22222222 11111111 00000000
				// Indexes:          A        A        A        A
				//                  B        B        B        B
				//                 C        C        C        D
				//                D        D        D        D
				//               E        E        E        E
				//              F        F        F        F
				//             G        G        G        G
				//            H        H        H        H
		    	bitplanes = (buffer_in[i + 2] << 24) + (buffer_in[i + 3] << 16) + (buffer_in[i + 0] << 8) + buffer_in[i + 1];
		    	
				for (p = 0; p < 8; p++) {
					color_index = ((bitplanes & 0x01000000UL) >> 21) |
									((bitplanes & 0x00010000UL) >> 14) |
									((bitplanes & 0x00000100UL) >> 7) |
									(bitplanes & 0x00000001UL);
					if (!(p & 1)) {
						byte = color_index << 4;
					} else {
						buffer_out[i + (p >> 1)] = byte | color_index;
					}
					bitplanes >>= 1;
				}
		    }
		} else {
	    	// Encode CD sprite tiles
		    for (i = 0; i < size; i++) {
				// Indexes:   AAAABBBB CCCCDDDD EEEEFFFF GGGGHHHH
				// Bitplanes:   1   1    1   1    1   1    1   1
				//               0   0    0   0    0   0    0   0
				//            3   3    3   3    3   3    3   3
				//             2   2    2   2    2   2    2   2
		    	indexes <<= 8;
		    	indexes |= buffer_in[i];
		    	
		    	if ((i & 3) == 3) {
		    		// Accumulated an 8-pixels row
					for (bp = 0; bp < 4; bp++) {
						mask = bitplane_masks[bp];
						bitplane = 0;
						for (p = 0; p < 8; p++) {
							bitplane <<= 1;
							if (indexes & mask)
								bitplane |= 1;
							mask <<= 4;
						}
						buffer_out[((i & 0xFFFC) ^ 0x40) + bp] = bitplane;
					}
				}
		    }
		}
	}

    file = fopen(argv[optind + 1], "wb");
    if (!file) {
       printf("Can't open output file.\n");
       return 1;
    }
    fwrite(buffer_out, 1, size, file);
    fclose(file);
    
	puts("Done.");
    return 0;  
}
