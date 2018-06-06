#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <png.h>
#include <getopt.h>

#define PREC 10

#define NUM_SQUARES 5

// Let's let val be 687. MAIN COLOR is val/768
// This takes the float value 'val', converts it to red, green & blue values, then
// sets those values into the image memory buffer location pointed to by 'ptr'
void set_rgb(png_byte *ptr, uint32_t val )
{
   ptr[0] = ( val >> 16 ) & 0xFFUL;   
   ptr[1] = ( val >> 8 ) & 0xFFUL;   
   ptr[2] = ( val ) & 0xFFUL;   
}

// This function actually writes out the PNG image file. The string 'title' is
// also written into the image file
int write_image(char* filename, int width, int height, uint32_t* buffer, char* title)
{
   int code = 0;
   FILE *fp = NULL;
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   png_bytep row = NULL;
   
   // Open file for writing (binary mode)
   fp = fopen(filename, "wb");
   if (fp == NULL) {
      fprintf(stderr, "Could not open file %s for writing\n", filename);
      code = 1;
      goto finalise;
   }

   // Initialize write structure
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (png_ptr == NULL) {
      fprintf(stderr, "Could not allocate write struct\n");
      code = 1;
      goto finalise;
   }

   // Initialize info structure
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      fprintf(stderr, "Could not allocate info struct\n");
      code = 1;
      goto finalise;
   }

   // Setup Exception handling
   if (setjmp(png_jmpbuf(png_ptr))) {
      fprintf(stderr, "Error during png creation\n");
      code = 1;
      goto finalise;
   }

   png_init_io(png_ptr, fp);

   // Write header (8 bit colour depth)
   png_set_IHDR(png_ptr, info_ptr, width, height,
      8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   // Set title
   if (title != NULL) {
      png_text title_text;
      title_text.compression = PNG_TEXT_COMPRESSION_NONE;
      title_text.key = "Title";
      title_text.text = title;
      png_set_text(png_ptr, info_ptr, &title_text, 1);
   }

   png_write_info(png_ptr, info_ptr);

   // Allocate memory for one row (3 bytes per pixel - RGB)
   row = (png_bytep) malloc(3 * width * sizeof(png_byte));

   // Write image data
   int x, y;
   for (y=0 ; y<height ; y++) {
      for (x=0 ; x<width ; x++) {
         set_rgb(&(row[x*3]), buffer[y*width + x]);
      }
      png_write_row(png_ptr, row);
   }

   // End write
   png_write_end(png_ptr, NULL);

   finalise:
   if (fp != NULL) fclose(fp);
   if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
   if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
   if (row != NULL) free(row);

   return code;
} // end of write_image()


void gen_square( uint32_t* pixels, double width, double height, double side_len, double x0, double y0, uint32_t color ) {
   double col_end = x0 + side_len;
   double row_end = y0 + side_len;
   for( double row = 0; row < height; row++ ) {
      for( double col = 0; col < width; col++ ) {
         // Does a square
         if ( ( col >= x0 ) &&
            ( col <= col_end ) &&
            ( row >= y0 ) &&
            ( row <= row_end )
         ) {

            int idx = ( int )( row * width + col );
            //pixels[ idx ] =  color + ((int)col & 0xff);
            pixels[ idx ] =  color;
         }
      } // for double col
   } // for double row

} // end of gen_square()


void gen_circle( uint32_t* pixels, double width, double height, 
   double radius, double x0, double y0, uint32_t color 
   ) {
   
   // Write to pixels
   for( double row = 0; row < height; row++ ) {
      for( double col = 0; col < width; col++ ) {
         double x_diff = col - x0;
         double y_diff = row - y0;
         double t_radius = sqrt( x_diff * x_diff + y_diff * y_diff );

         int idx = ( int )( row * width + col );
         if ( t_radius <= radius ) {
            pixels[ idx ] = color;
         }
      } // for col
   } // for row

} // end of gen_circle()


#     define VERBOSE_PRINTF(fmt, ...) { \
         if ( verbose_flag ) { \
            printf(fmt, ##__VA_ARGS__); \
         } \
      }


static int verbose_flag;


static struct option long_options[] = {
   { "width", required_argument, NULL, 'w' },
   { "height", required_argument, NULL, 'h' },
   { "side_length", required_argument, NULL, 's' },
   { "color", optional_argument, NULL, 'c' },
   { "outfile", optional_argument, NULL, 'o' },
   { "verbose", no_argument, &verbose_flag, 1 },
   { 0, 0, 0, 0 }
};   

void usage( char* argv ) {
   printf( "Usage %s <options>\n", argv );
   printf( "options is one of:" ); 
   printf( "%7s %3s %s\n", 
      "--width", "-w", "width of the image to be generated." );  
   printf( "%7s %3s %s\n", 
      "--height", "-h", "height of the image to be generated." );  
   printf( "%7s %3s %s\n",
      "--side_length", "-s", "length of a side of the largest square in the generated image." );  
   printf( "%7s %3s %s\n",
      "--color", "-c", "packed RGB (rightmost 24 bits of 32-bit) value for the color of the circle in the generated image. (optional)" );  
   printf( "%7s %3s %s\n", 
      "--outfile", "-o", "name for the PNG file generated (optional)." );  
   printf( "%7s %3s %s\n\n", 
      "--verbose", "-v", "show more display statements (optional)" );  
}

int main( int argc, char **argv ) {
   double width = 0.0;
   double height = 0.0;
   double side_length = 0.0;
   
   uint32_t color = 0;

   char outfile[64];
   char* endptr = NULL;

   char ch;
   int option_index = 0;
   strcpy( outfile, "square.png" );
   while( (ch = getopt_long( argc, argv, "w:h:s:c:o:v", 
      long_options, &option_index )) != -1 ) {
      
      switch( ch ) {
         case 0:
            // For verbose flag 
            // There is no argument after the flag in this case
            if ( long_options[option_index].flag != 0 ) {
               break;
            }
         case 'w':
            width = strtod( optarg, &endptr );
            break;
         case 'h':
            height = strtod( optarg, &endptr );
            break;
         case 's':
            side_length = strtod( optarg, &endptr );
            break;
         case 'c':
            color = (uint32_t)strtoul( optarg, &endptr, 16 );
            break;
         case 'o':
            strcpy( outfile, optarg );
            break;
         default:
            printf( "ERROR: option %c invalid\n", ch );
            usage( argv[0] ); 
            exit( EXIT_FAILURE );
      }
   }

	if ( width == 0.0 ) {
		printf( "ERROR: width is 0.0. Invalid input.\n" );
		usage( argv[0] );
		exit( EXIT_FAILURE );
	}
	if ( height == 0.0 ) {
		printf( "ERROR: height is 0.0. Invalid input.\n" );
		usage( argv[0] );
		exit( EXIT_FAILURE );
	}
	if ( side_length == 0.0 ) {
		printf( "ERROR: side_length is 0.0. Invalid input.\n" );
		usage( argv[0] );
		exit( EXIT_FAILURE );
	}

   char title[64];
   strcpy( title, "Square" );
   uint32_t black = 0;
   uint32_t white = 0x00FFFFFFUL;

   uint32_t* pixels = calloc( ( height * width ),  sizeof( uint32_t ) ); 

   for( int i = 0; i < ( width * height ); i++ ) {
      pixels[i] = white;
   } 

   double sq_x0 = 0.10 * side_length;
   double sq_y0 = 0.10 * side_length;

   int colors[NUM_SQUARES] = {
      0x0408ff,
      0x24057c,
      0x6478ff,
      0xcc2200,
      0xfd2ce0
   };

   double factors[NUM_SQUARES] = {
      3.0,
      2.5,
      2.0,
      1.5, 
      1.0 
   };
   VERBOSE_PRINTF( "There will be %f points on the x-axis\n", width );  
   VERBOSE_PRINTF( "There will be %f points on the y-axis\n", height );  
   VERBOSE_PRINTF( "Each side of the main square will have %d pixels\n",
      (int)side_length );
   VERBOSE_PRINTF( "There will be %d squares\n", NUM_SQUARES );

   char color_strings[64];
   for ( int index = 0; index  < NUM_SQUARES; index++ ) {
      sprintf( color_strings, "%s%x%s ", color_strings, colors[index],
         ( (index == NUM_SQUARES - 2) ? " and" : ", " ) );
   }
   VERBOSE_PRINTF( "The colors will be %s\n", color_strings );
   printf( "Generating data for %s...\n", title ); 

   double side_lengths[NUM_SQUARES];
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      side_lengths[index] = side_length/factors[index];
   } 

   double x0s[NUM_SQUARES];
   x0s[0] = sq_x0;
   for( int index = 1; index < NUM_SQUARES; index++ ) {
      x0s[index] = sq_x0 + side_lengths[index-1] + x0s[index-1];
   } 

   double y0s[NUM_SQUARES];
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      y0s[index] = sq_y0;
   } 

   for( int index = 0; index < NUM_SQUARES; index++ ) {
      gen_square( pixels, width, height, side_lengths[index], x0s[index], 
         y0s[index], colors[index] );
   } 
   
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      y0s[index] = ( 2 * sq_y0 ) + side_length ;
   } 
   
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      gen_square( pixels, width, height, side_lengths[index], x0s[index], 
         y0s[index], colors[index] );
   } 
   
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      y0s[index] = ( 3 * sq_y0 ) + ( 2 * side_length );
   } 
   
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      gen_square( pixels, width, height, side_lengths[index], x0s[index], 
         y0s[index], colors[index] );
   } 
   
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      y0s[index] = ( 4 * sq_y0 ) + ( 3 * side_length );
   } 
   
   for( int index = 0; index < NUM_SQUARES; index++ ) {
      gen_square( pixels, width, height, side_lengths[index], x0s[index], 
         y0s[index], colors[index] );
   } 
   
   printf( "Saving PNG to %s...\n", outfile ); 
   write_image( outfile, width, height, pixels, title );
   printf( "DONE.\n\n" ); 

	exit( EXIT_SUCCESS );
}

