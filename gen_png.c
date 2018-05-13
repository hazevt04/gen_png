#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <png.h>
#include <getopt.h>

#define PREC 10

// Let's let val be 687. MAIN COLOR is val/768
#define OTHER_COLOR 1.0              
#define MAIN_COLOR  0.0               

// This takes the float value 'val', converts it to red, green & blue values, then
// sets those values into the image memory buffer location pointed to by 'ptr'
void set_rgb(png_byte *ptr, double val)
{
   int v = (int)(val * 767);
   if (v < 0) v = 0;
   if (v > 767) v = 767;
   int offset = v % 256;

   if (v<256) {
      ptr[0] = 0; 
      ptr[1] = 0; 
      ptr[2] = offset;
   }
   else if (v<512) {
      ptr[0] = 0; 
      ptr[1] = offset; 
      ptr[2] = 255-offset;
   }
   else {
      ptr[0] = offset; 
      ptr[1] = 255-offset; 
      //ptr[2] = 0;
      ptr[2] = 255;
   }
}

// This function actually writes out the PNG image file. The string 'title' is
// also written into the image file
int write_image(char* filename, int width, int height, double *buffer, char* title)
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


void gen_square( double* pixels, double width, double height ) {
   
   for( double row = 0; row < height; row++ ) {
      for( double col = 0; col < width; col++ ) {
         // Does a square
         int idx = ( int )( row * width + col );
         if ( 
            ( row > 1250 ) && 
            ( row < 3750 ) &&
            ( col > 1250 ) && 
            ( col < 3750 )
         ) {
            pixels[ idx ] = MAIN_COLOR;
         } else {
            pixels[ idx ] = OTHER_COLOR;
         }
      } // for double col
   } // for double row

} // end of gen_square()


void gen_circle( double* pixels, double width, double height, 
   double radius, double x0, double y0 
   ) {
   
   // Write to pixels
   for( double row = 0; row < height; row++ ) {
      for( double col = 0; col < width; col++ ) {
         double x_diff = col - x0;
         double y_diff = row - y0;
         double t_radius = sqrt( x_diff * x_diff + y_diff * y_diff );

         int idx = ( int )( row * width + col );
         if ( t_radius <= radius ) {
            pixels[ idx ] = MAIN_COLOR;
         } else {
            pixels[ idx ] = OTHER_COLOR;
         }
      } // for col
   } // for row

} // end of gen_circle()


static int verbose_flag;


static struct option long_options[] = {
   { "width", required_argument, NULL, 'w' },
   { "height", required_argument, NULL, 'h' },
   { "radius", required_argument, NULL, 'r' },
   { "verbose", no_argument, &verbose_flag, 1 },
   { 0, 0, 0, 0 }
};   

void usage( char* argv ) {
   printf( "Usage %s <options>\n", argv );
   printf( "width, w\n" );  
   printf( "height, h\n" );  
   printf( "radius, h\n" );  
   printf( "verbose, v\n" );  
}

int main( int argc, char **argv ) {
   double width = 0.0;
   double height = 0.0;
   double radius = 0.0;
   char* endptr = NULL;

   char ch;
   while( (ch = getopt_long( argc, argv, "w:h:r:v", long_options, NULL )) != -1 ) {
      switch( ch ) {
         case 'w':
            width = strtod( optarg, &endptr );
            break;
         case 'h':
            height = strtod( optarg, &endptr );
            break;
         case 'r':
            radius = strtod( optarg, &endptr );
            break;
         default:
            printf( "ERROR: option %c invalid", ch );
            usage( argv[0] ); 
            break; 
      }
   }

   double y0 = height/2.0;
   double x0 = width/2.0;

   printf( "The center is at %f, %f\n", x0, y0 ); 
   printf( "Radius is %f\n", radius ); 
   printf( "There will be %f points on the x-axis\n", ( width ) );  
   printf( "There will be %f points on the y-axis\n", ( height ) );  
   double* pixels = calloc( ( height * width ),  sizeof( double ) ); 
   
   gen_circle( pixels, width, height, radius, x0, y0 );

   char filename[64];
   char title[64];
   strcpy( filename, "circle.png" );
   strcpy( title, "Circle" );
   printf( "Saving PNG...\n" ); 
   write_image( filename, width, height, pixels, title );
   printf( "DONE. Result is in %s\n", filename ); 

   return 0;
}

