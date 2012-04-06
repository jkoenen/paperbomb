#include <stdio.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

unsigned char ttf_buffer[1<<25];


int main(int argc,char** argv)
{
   stbtt_fontinfo font;

   fread(ttf_buffer, 1, 1<<25, fopen( "/home/julien/Projects/paperbomb/tools/fontgen/fonts/Ubuntu-Regular.ttf", "rb"));

    stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0));

    float advance[255];
    //int point
    int firstC=33;
    int lastC=122;

    for(int cc=firstC;cc<=lastC;++cc)
    {
        int gi=stbtt_FindGlyphIndex(&font,cc);
        
        stbtt_vertex* pVertices;
        int pc=stbtt_GetGlyphShape(&font, gi, &pVertices);

        int charAdvance;
        int leftSideBearing;
        stbtt_GetCodepointHMetrics(&font, cc, &charAdvance, &leftSideBearing);
        advance[cc]=charAdvance/256.0f;

        printf("static const float2 s_font_points_%i[] =\n{", cc);
        for(int i=0;i<pc;++i)
        {
            if(pVertices[i].type==STBTT_vcurve)
            {
                printf("{%ff,%ff},", (float)pVertices[i].cx/256.0f,(float)pVertices[i].cy/256.0f);
            }
            printf("{%ff,%ff}", pVertices[i].x/256.0f,pVertices[i].y/256.0f);

            if(i+1<pc)
            {
                printf(",");
            }
        }      
        printf( "};\n" );
        
        printf("static const uint8 s_font_commands_%i[] =\n{", cc);
        for(int i=0;i<pc;++i)
        {
            switch(pVertices[i].type)
            {
            case STBTT_vmove:
                printf("DrawCommand_Move");
                break;

            case STBTT_vline:
                printf("DrawCommand_Line");
                break;

            case STBTT_vcurve:
                printf("DrawCommand_Curve");
                break;
            }
            if(i+1<pc)
            {
                printf(",");
            }
        }      
        printf( "};\n" );

        
        /*printf("gi=%i\n",gi);
        printf("pc=%i\n",pc);

        for(int i=0;i<pc;++i)
        {
            printf("%i,%i,%i,%i (%i)\n", pVertices[i].x, pVertices[i].y, pVertices[i].cx, pVertices[i].cy, pVertices[i].type);
        }      */
    }

    printf("\nstatic const FontGlyph s_glyphs[] =\n{");
    for(int cc=firstC;cc<=lastC;++cc)
    {
        printf("{%i,%ff,s_font_points_%i,s_font_commands_%i,SYS_COUNTOF(s_font_commands_%i)}",
            cc,advance[cc],cc,cc,cc);
        if(cc<lastC)
        {
            printf(",\n");
        }
    }
    printf("\n};\n");
/*    bitmap = stbtt_GetCodepointBitmap(&font, 0,stbtt_ScaleForPixelHeight(&font, s), c, &w, &h, 0,0);

   for (j=0; j < h; ++j) {
      for (i=0; i < w; ++i)
         putchar(" .:ioVM@"[bitmap[j*w+i]>>5]);
      putchar('\n');
   }*/
   return 0;
}



