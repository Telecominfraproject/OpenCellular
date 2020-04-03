// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    using System;

    /// <summary>Represents Class BivariateInterpolation.</summary>
    public class BivariateInterpolation
    {
        /// <summary>Interpolates the specified lx.</summary>
        /// <param name="lx">NUMBER OF INPUT GRID POINTS IN THE X COORDINATE (MUST BE 2 OR GREATER).</param>
        /// <param name="ly">NUMBER OF INPUT GRID POINTS IN THE Y COORDINATE (MUST BE 2 OR GREATER).</param>
        /// <param name="x">ARRAY OF DIMENSION LX STORING THE X COORDINATES OF INPUT GRID POINTS (IN ASCENDING ORDER).</param>
        /// <param name="y">ARRAY OF DIMENSION LY STORING THE Y COORDINATES OF INPUT GRID POINTS (IN ASCENDING ORDER).</param>
        /// <param name="z">DOUBLY-DIMENSIONED ARRAY OF DIMENSION (LX,LY) STORING THE VALUES OF THE FUNCTION (Z VALUES) AT INPUT GRID POINTS.</param>
        /// <param name="npoints">NUMBER OF POINTS AT WHICH INTERPOLATION OF THE Z VALUE IS DESIRED (MUST BE 1 OR GREATER).</param>
        /// <param name="u">ARRAY OF DIMENSION N STORING THE X COORDINATES OF DESIRED POINTS.</param>
        /// <param name="v">ARRAY OF DIMENSION N STORING THE Y COORDINATES OF DESIRED POINTS.</param>
        /// <param name="w">ARRAY OF DIMENSION N WHERE THE INTERPOLATED Z VALUES AT DESIRED POINTS ARE TO BE DISPLAYED.</param>
        public void Interpolate(int lx, int ly, FArray x, FArray y, F2DArray z, int npoints, FArray u, FArray v, FArray w)
        {
            // BIVARIATE INTERPOLATION
            // THIS SUBROUTINE INTERPOLATES, FROM VALUES OF THE FUNCTION
            // GIVEN AT INPUT GRID POINTS IN AN X-Y PLANE AND FOR A GIVEN
            // SET OF POINTS IN THE PLANE, THE VALUES OF A SINGLE-VALUED
            // BIVARIATE FUNCTION Z = Z(X,Y).
            // THE METHOD IS BASED ON A PIECE-WISE FUNCTION COMPOSED OF
            // A SET OF BICUBIC POLYNOMIALS IN X AND Y.  EACH POLYNOMIAL
            // IS APPLICABLE TO A RECTANGLE OF THE INPUT GRID IN THE X-Y
            // PLANE.  EACH POLYNOMIAL IS DETERMINED LOCALLY.
            // THE INPUT PARAMETERS ARE
            // LX  = NUMBER OF INPUT GRID POINTS IN THE X COORDINATE
            //       (MUST BE 2 OR GREATER)
            // LY  = NUMBER OF INPUT GRID POINTS IN THE Y COORDINATE
            //       (MUST BE 2 OR GREATER)
            // X   = ARRAY OF DIMENSION LX STORING THE X COORDINATES
            //       OF INPUT GRID POINTS (IN ASCENDING ORDER)
            // Y   = ARRAY OF DIMENSION LY STORING THE Y COORDINATES
            //       OF INPUT GRID POINTS (IN ASCENDING ORDER)
            // Z   = DOUBLY-DIMENSIONED ARRAY OF DIMENSION (LX,LY)
            //       STORING THE VALUES OF THE FUNCTION (Z VALUES)
            //       AT INPUT GRID POINTS
            // N   = NUMBER OF POINTS AT WHICH INTERPOLATION OF THE
            //       Z VALUE IS DESIRED (MUST BE 1 OR GREATER)
            // U   = ARRAY OF DIMENSION N STORING THE X COORDINATES
            //       OF DESIRED POINTS
            // V   = ARRAY OF DIMENSION N STORING THE Y COORDINATES
            //       OF DESIRED POINTS
            // THE OUTPUT PARAMETER IS
            // W   = ARRAY OF DIMENSION N WHERE THE INTERPOLATED Z
            //       VALUES AT DESIRED POINTS ARE TO BE DISPLAYED
            //       SOME VARIABLES INTERNALLY USED ARE
            // ZA  = DIVIDED DIFFERENCE OF Z WITH RESPECT TO X
            // ZB  = DIVIDED DIFFERENCE OF Z WITH RESPECT TO Y
            // ZAB = SECOND ORDER DIVIDED DIFFERENCE OF Z WITH;
            //       RESPECT TO X AND Y
            // ZX  = PARTIAL DERIVATIVE OF Z WITH RESPECT TO X
            // ZY  = PARTIAL DERIVATIVE OF Z WITH RESPECT TO Y
            // ZXY = SECOND ORDER PARTIAL DERIVATIVE OF Z WITH
            //       RESPECT TO X AND Y

            // variable declaration
            int k, jxml, jyml;
            float x3, a3, y3, b3;
            float sw = 0.0f, zx3b3 = 0.0f, zx4b3 = 0.0f, zy3a3 = 0.0f, zy4a3 = 0.0f;

            k = jxml = jyml = 0;
                
            x3 = a3 = y3 = b3 = 0.0f;

            ZA za = new ZA(5, 2);
            ZB zb = new ZB(2, 5);
            ZAB zab = new ZAB(3, 3);
            ZX zx = new ZX(4, 4);
            ZY zy = new ZY(4, 4);
            ZXY zxy = new ZXY(4, 4);

            SharedVariables sh = new SharedVariables();

            zx.LX0 = lx;
            zx.LXM1 = zx.LX0 - 1;
            zx.LXM2 = zx.LXM1 - 1;
            zx.LXP1 = zx.LX0 + 1;
            zy.LY0 = ly;
            zy.LYM1 = zy.LY0 - 1;
            zy.LYM2 = zy.LYM1 - 1;
            zy.LYP1 = zy.LY0 + 1;

            zxy.IXPV = 0;
            zxy.IYPV = 0;

            // main loop
            for (k = 1; k <= npoints; k++)
            {
                sh.UK = u[k];
                sh.VK = v[k];
                
                // (U(K).GE.X(IX-1)).AND.(U(K).LT.X(IX))
                if (zx.LXM2 == 0)
                {
                    goto lbl80;
                }

                if (sh.UK >= x[zx.LX0])
                {
                    goto lbl70;
                }

                if (sh.UK < x[1])
                {
                    goto lbl60;
                }

                sh.IMN = 2;
                sh.IMX = zx.LX0;

            lbl30:
                zxy.IX = (sh.IMN + sh.IMX) / 2;

                // UK.GE.X(IX)
                if (sh.UK >= x[zxy.IX])
                {
                    goto lbl40;
                }

                sh.IMX = zxy.IX;
                goto lbl50;

            lbl40:
                sh.IMN = zxy.IX + 1;

            lbl50:
                if (sh.IMX > sh.IMN)
                {
                    goto lbl30;
                }

                zxy.IX = sh.IMX; // continue to lbl90
                goto lbl90;

            lbl60:
                zxy.IX = 1;
                goto lbl90;

            lbl70:
                zxy.IX = zx.LXP1; // goto lbl90
                goto lbl90;

            lbl80:
                zxy.IX = 2;

            ////  TO FIND OUT THE IY VALUE FOR WHICH 
            ////  (V(K).GE.Y(IY-1)).AND.(V(K).LT.Y(IY)) 
            lbl90:
                if (zy.LYM2 == 0)
                {
                    goto lbl150;
                }

                // VK.GE.Y(LY0)
                if (sh.VK >= y[zy.LY0])
                {
                    goto lbl140;
                }

                if (sh.VK < y[1])
                {
                    goto lbl130;
                }

                sh.IMN = 2;
                sh.IMX = zy.LY0;

            lbl100:
                zxy.IY = (sh.IMN + sh.IMX) / 2;
                if (sh.VK >= y[zxy.IY])
                {
                    goto lbl110;
                }

                sh.IMX = zxy.IY;
                goto lbl120;

            lbl110:
                sh.IMN = zxy.IY + 1;

            lbl120:
                if (sh.IMX > sh.IMN)
                {
                    goto lbl100;
                }

                zxy.IY = sh.IMX;
                goto lbl160;

            lbl130:
                zxy.IY = 1;
                goto lbl160;

            lbl140:
                zxy.IY = zy.LYP1;
                goto lbl160;

            lbl150:
                zxy.IY = 2;

            ////  TO CHECK IF THE DESIRED POINT IS IN THE SAME RECTANGLE 
            ////  AS THE PREVIOUS POINT. IF YES, SKIP TO THE COMPUTATION 
            ////  OF THE POLYNOMIAL 

                lbl160:
                if (zxy.IX == zxy.IXPV && zxy.IY == zxy.IYPV)
                {
                    goto lbl690;
                }

                zxy.IXPV = zxy.IX;
                zxy.IYPV = zxy.IY;

                ////  ROUTINES TO PICK UP NECESSARY X,Y, AND Z VALUES, TO 
                ////  COMPUTE THE ZA, ZB AND ZAB VALUES, AND TO ESTIMATE THEM 
                ////  WHEN NECESSARY 

                sh.JX = zxy.IX;
                if (sh.JX == 1)
                {
                    sh.JX = 2;
                }

                // IF (JX.EQ.LXP1) JX = LX0
                if (sh.JX == zx.LXP1)
                {
                    sh.JX = zx.LX0;
                }

                sh.JY = zxy.IY;

                // IF (JY.EQ.1) JY = 2
                if (sh.JY == 1)
                {
                    sh.JY = 2;
                }

                // IF (JY.EQ.LYP1) JY = LY0
                if (sh.JY == zy.LYP1)
                {
                    sh.JY = zy.LY0;
                }

                sh.JXM2 = sh.JX - 2;
                jxml = sh.JX - zx.LX0;
                sh.JYM2 = sh.JY - 2;
                jyml = sh.JY - zy.LY0;

                ////  IN THE CORE AREA, I.E., IN THE RECTANGLE THAT CONTAINS
                ////  THE DESIRED POINT

                x3 = x[sh.JX - 1];
                zx.X4 = x[sh.JX]; // X4 = X(JX)
                a3 = 1.0f / (zx.X4 - x3); // A3 = 1/(X4-X3) 
                if (float.IsNaN(a3))
                {
                    a3 = 0.0f;
                }

                y3 = y[sh.JY - 1]; // Y3 = Y(JY-1) 
                zy.Y4 = y[sh.JY]; // Y4 = Y(JY)
                b3 = 1.0f / (zy.Y4 - y3); // B3 = 1/(Y4-Y3)
                if (float.IsNaN(b3))
                {
                    b3 = 0.0f;
                }

                sh.Z33 = z[sh.JX - 1, (sh.JY - 1)]; // Z(JX-1,JY-1)

                zxy.Z43 = z[sh.JX, sh.JY - 1]; // Z43 = Z(JX,JY-1)

                zy.Z34 = z[sh.JX - 1, sh.JY]; // Z34 = Z(JX-1,JY)

                zxy.Z44 = z[sh.JX, sh.JY]; // Z(JX,JY)

                za.Z3A3 = (zxy.Z43 - sh.Z33) * a3; // Z3A3 = (Z43-Z33)*A3

                za.Z4A3 = (zxy.Z44 - zy.Z34) * a3; // Z4A3 = (Z44-Z34)*A3

                zb.Z3B3 = (zy.Z34 - sh.Z33) * b3; // Z3B3 = (Z34-Z33)*B3

                zb.Z4B3 = (zxy.Z44 - zxy.Z43) * b3; // Z4B3 = (Z44-Z43)*B3

                zab.ZA3B3 = (zb.Z4B3 - zb.Z3B3) * a3; // ZA3B3 = (Z4B3-Z3B3)*A3

                ////  IN THE X DIRECTION 

                // IF (LXM2.EQ.0) GO TO 230
                if (zx.LXM2 == 0)
                {
                    goto lbl230;
                }

                // IF (JXM2.EQ.0) GO TO 170
                if (sh.JXM2 == 0)
                {
                    goto lbl170;
                }

                zx.X2 = x[sh.JX - 2]; // X2 = X(JX-2)
                zx.A2 = 1.0f / (x3 - zx.X2); // A2 = 1/(X3-X2)
                if (float.IsNaN(zx.A2))
                {
                    zx.A2 = 0.0f;
                }

                zy.Z23 = z[sh.JX - 2, sh.JY - 1]; // Z23 = Z(JX-2,JY-1)
                zy.Z24 = z[sh.JX - 2, sh.JY]; // Z24 = Z(JX-2,JY)
                za.Z3A2 = (sh.Z33 - zy.Z23) * zx.A2; // Z3A2 = (Z33-Z23)*A2
                za.Z4A2 = (zy.Z34 - zy.Z24) * zx.A2; // Z4A2 = (Z34-Z24)*A2

                // IF (JXML.EQ.0) GO TO 180
                if (jxml == 0)
                {
                    goto lbl180;
                }

            lbl170:
                zx.X5 = x[sh.JX + 1]; // X5 = X(JX+1)
                zx.A4 = 1.0f / (zx.X5 - zx.X4); // A4 = 1/(X5-X4)
                if (float.IsNaN(zx.A4))
                {
                    zx.A4 = 0.0F;
                }

                zxy.Z53 = z[sh.JX + 1, sh.JY - 1]; // Z53 = Z(JX+1,JY-1)
                zxy.Z54 = z[sh.JX + 1, sh.JY]; // Z54 = Z(JX+1,JY)
                za.Z3A4 = (zxy.Z53 - zxy.Z43) * zx.A4; // Z3A4 = (Z53-Z43)*A4
                za.Z4A4 = (zxy.Z54 - zxy.Z44) * zx.A4; // Z4A4 = (Z54-Z44)*A4

                if (sh.JXM2 != 0)
                {
                    goto lbl190; // IF (JXM2.NE.0) GO TO 190
                }

                za.Z3A2 = za.Z3A3 + za.Z3A3 - za.Z3A4; // Z3A2 = Z3A3 + Z3A3 - Z3A4
                za.Z4A2 = za.Z4A3 + za.Z4A3 - za.Z4A4; // Z4A2 = Z4A3 + Z4A3 - Z4A4

                goto lbl190;

            lbl180:
                za.Z3A4 = za.Z3A3 + za.Z3A3 - za.Z3A2; // Z3A4 = Z3A3 + Z3A3 - Z3A2
                za.Z4A4 = za.Z4A3 + za.Z4A3 - za.Z4A2; // Z4A4 = Z4A3 + Z4A3 - Z4A2

                lbl190:
                zab.ZA2B3 = (za.Z4A2 - za.Z3A2) * b3; // ZA2B3 = (Z4A2-Z3A2)*B3
                zab.ZA4B3 = (za.Z4A4 - za.Z3A4) * b3; // ZA4B3 = (Z4A4-Z3A4)*B3       

                // IF (JX.LE.3) GO TO 200
                if (sh.JX <= 3)
                {
                    goto lbl200;
                }

                zx.A1 = 1.0f / (zx.X2 - x[sh.JX - 3]); // A1 = 1/(X2-X(JX-3))
                if (float.IsNaN(zx.A1))
                {
                    zx.A1 = 0.0f;
                }

                za.Z3A1 = (zy.Z23 - z[sh.JX - 3, sh.JY - 1]) * zx.A1; // Z3A1 = (Z23-Z(JX-3,JY-1))*A1
                za.Z4A1 = (zy.Z24 - z[sh.JX - 3, sh.JY]) * zx.A1; // Z4A1 = (Z24-Z(JX-3,JY))*A1

                goto lbl210;

            lbl200:
                za.Z3A1 = za.Z3A2 + za.Z3A2 - za.Z3A3; // Z3A1 = Z3A2 + Z3A2 - Z3A3
                za.Z4A1 = za.Z4A2 + za.Z4A2 - za.Z4A3; // Z4A1 = Z4A2 + Z4A2 - Z4A3

                lbl210:

                // IF (JX.GE.LXM1) GO TO 220
                if (sh.JX >= zx.LXM1)
                {
                    goto lbl220;
                }

                zx.A5 = 1.0f / (x[sh.JX + 2] - zx.X5); // A5 = 1/(X(JX+2)-X5)
                if (float.IsNaN(zx.A5))
                {
                    zx.A5 = 0.0f;
                }

                za.Z3A5 = (z[sh.JX + 2, sh.JY - 1] - zxy.Z53) * zx.A5; // Z3A5 = (Z(JX+2,JY-1)-Z53)*A5
                za.Z4A5 = (z[sh.JX + 2, sh.JY] - zxy.Z54) * zx.A5; // Z4A5 = (Z(JX+2,JY)-Z54)*A5

                goto lbl240;

            lbl220:
                za.Z3A5 = za.Z3A4 + za.Z3A4 - za.Z3A3; // Z3A5 = Z3A4 + Z3A4 - Z3A3
                za.Z4A5 = za.Z4A4 + za.Z4A4 - za.Z4A3; // Z4A5 = Z4A4 + Z4A4 - Z4A3

                goto lbl240;

            lbl230:
                za.Z3A2 = za.Z3A3; // Z3A2 = Z3A3
                za.Z4A2 = za.Z4A3; // Z4A2 = Z4A3

                goto lbl180;

            //// IN THE Y DIRECTION

            lbl240:

                // IF (LYM2.EQ.0) GO TO 310
                if (zy.LYM2 == 0)
                {
                    goto lbl310;
                }

                // IF (JYM2.EQ.0) GO TO 250
                if (sh.JYM2 == 0)
                {
                    goto lbl250;
                }

                zx.Y2 = y[sh.JY - 2]; // Y2 = Y(JY-2)
                zy.B2 = 1.0f / (y3 - zx.Y2); // B2 = 1/(Y3-Y2)
                if (float.IsNaN(zy.B2))
                {
                    zy.B2 = 0.0f;
                }

                zy.Z32 = z[sh.JX - 1, sh.JY - 2]; // Z32 = Z(JX-1,JY-2)
                zxy.Z42 = z[sh.JX, sh.JY - 2]; // Z42 = Z(JX,JY-2)
                zb.Z3B2 = (sh.Z33 - zy.Z32) * zy.B2; // Z3B2 = (Z33-Z32)*B2
                zb.Z4B2 = (zxy.Z43 - zxy.Z42) * zy.B2; // Z4B2 = (Z43-Z42)*B2

                // IF (JYML.EQ.0) GO TO 260
                if (jyml == 0)
                {
                    goto lbl260;
                }

            lbl250:
                zx.Y5 = y[sh.JY + 1]; ////  Y5 = Y(JY+1)
                zy.B4 = 1.0f / (zx.Y5 - zy.Y4); // B4 = 1/(Y5-Y4)
                if (float.IsNaN(zy.B4))
                {
                    zy.B4 = 0.0f;
                }

                zy.Z35 = z[sh.JX - 1, sh.JY + 1]; // Z(JX-1,JY+1)
                zxy.Z45 = z[sh.JX, sh.JY + 1]; // Z45 = Z(JX,JY+1)
                zb.Z3B4 = (zy.Z35 - zy.Z34) * zy.B4; // Z3B4 = (Z35-Z34)*B4
                zb.Z4B4 = (zxy.Z45 - zxy.Z44) * zy.B4; // Z4B4 = (Z45-Z44)*B4

                // IF (JYM2.NE.0) GO TO 270
                if (sh.JYM2 != 0)
                {
                    goto lbl270;
                }

                zb.Z3B2 = zb.Z3B3 + zb.Z3B3 - zb.Z3B4; // Z3B2 = Z3B3 + Z3B3 - Z3B4
                zb.Z4B2 = zb.Z4B3 + zb.Z4B3 - zb.Z4B4; // Z4B2 = Z4B3 + Z4B3 - Z4B4

                goto lbl270;

            lbl260:
                zb.Z3B4 = zb.Z3B3 + zb.Z3B3 - zb.Z3B2; // Z3B4 = Z3B3 + Z3B3 - Z3B2
                zb.Z4B4 = zb.Z4B3 + zb.Z4B3 - zb.Z4B2; // Z4B4 = Z4B3 + Z4B3 - Z4B2

                lbl270:
                zab.ZA3B2 = (zb.Z4B2 - zb.Z3B2) * a3; // ZA3B2 = (Z4B2-Z3B2)*A3
                zab.ZA3B4 = (zb.Z4B4 - zb.Z3B4) * a3; // ZA3B4 = (Z4B4-Z3B4)*A3

                if (sh.JY <= 3)
                {
                    goto lbl280; // IF (JY.LE.3) GO TO 280
                }

                zx.B1 = 1.0f / (zx.Y2 - y[sh.JY - 3]); // B1 = 1/(Y2-Y(JY-3))
                if (float.IsNaN(zx.B1))
                {
                    zx.B1 = 0.0f;
                }
                
                zb.Z3B1 = (zy.Z32 - z[sh.JX - 1, sh.JY - 3]) * zx.B1; // Z3B1 = (Z32-Z(JX-1,JY-3))*B1
                zb.Z4B1 = (zxy.Z42 - z[sh.JX, sh.JY - 3]) * zx.B1; // Z4B1 = (Z42-Z(JX,JY-3))*B1

                goto lbl290;

            lbl280:
                zb.Z3B1 = zb.Z3B2 + zb.Z3B2 - zb.Z3B3; // Z3B1 = Z3B2 + Z3B2 - Z3B3
                zb.Z4B1 = zb.Z4B2 + zb.Z4B2 - zb.Z4B3; // Z4B1 = Z4B2 + Z4B2 - Z4B3

                lbl290:

                // IF (JY.GE.LYM1) GO TO 300
                if (sh.JY >= zy.LYM1)
                {
                    goto lbl300;
                }

                zx.B5 = 1.0f / (y[sh.JY + 2] - zx.Y5); // B5 = 1/(Y(JY+2)-Y5)
                if (float.IsNaN(zx.B5))
                {
                    zx.B5 = 0.0f;
                }

                zb.Z3B5 = (z[sh.JX - 1, sh.JY + 2] - zy.Z35) * zx.B5; // Z3B5 = (Z(JX-1,JY+2)-Z35)*B5
                zb.Z4B5 = (z[sh.JX, sh.JY + 2] - zxy.Z45) * zx.B5; // Z4B5 = (Z(JX,JY+2)-Z45)*B5

                goto lbl320;

            lbl300:
                zb.Z3B5 = zb.Z3B4 + zb.Z3B4 - zb.Z3B3; // Z3B5 = Z3B4 + Z3B4 - Z3B3
                zb.Z4B5 = zb.Z4B4 + zb.Z4B4 - zb.Z4B3; // Z4B5 = Z4B4 + Z4B4 - Z4B3

                goto lbl320;

            lbl310:
                zb.Z3B2 = zb.Z3B3; // Z3B2 = Z3B3
                zb.Z4B2 = zb.Z4B3; // Z4B2 = Z4B3

                goto lbl260;

            //// IN THE DIAGONAL DIRECTIONS

            lbl320:
               
                // IF (LXM2.EQ.0) GO TO 400
                if (zx.LXM2 == 0) 
                {
                    goto lbl400;
                }

                // IF (LYM2.EQ.0) GO TO 410
                if (zy.LYM2 == 0) 
                {
                    goto lbl410;
                }

                // IF (JXML.EQ.0) GO TO 350
                if (jxml == 0) 
                {
                    goto lbl350;
                }

                // IF (JYM2.EQ.0) GO TO 330
                if (sh.JYM2 == 0) 
                {
                    goto lbl330;
                }

                zab.ZA4B2 = (((zxy.Z53 - z[sh.JX + 1, sh.JY - 2]) * zy.B2) - zb.Z4B2) * zx.A4; // ZA4B2 = ((Z53-Z(JX+1,JY-2))*B2-Z4B2)*A4

                // IF (JYML.EQ.0) GO TO 340
                if (jyml == 0) 
                {
                    goto lbl340;
                }

            lbl330:
                zab.ZA4B4 = (((z[sh.JX + 1, sh.JY + 1] - zxy.Z54) * zy.B4) - zb.Z4B4) * zx.A4; // ZA4B4 = ((Z(JX+1,JY+1)-Z54)*B4-Z4B4)*A4

                // IF (JYM2.NE.0) GO TO 380
                if (sh.JYM2 != 0) 
                {
                    goto lbl380;
                }

                zab.ZA4B2 = zab.ZA4B3 + zab.ZA4B3 - zab.ZA4B4; // ZA4B2 = ZA4B3 + ZA4B3 - ZA4B4

                goto lbl380;

            lbl340:
                zab.ZA4B4 = zab.ZA4B3 + zab.ZA4B3 - zab.ZA4B2; // ZA4B4 = ZA4B3 + ZA4B3 - ZA4B2
                goto lbl380;

            lbl350:

                // IF (JYM2.EQ.0) GO TO 360
                if (sh.JYM2 == 0) 
                {
                    goto lbl360;
                }

                zab.ZA2B2 = (zb.Z3B2 - ((zy.Z23 - z[sh.JX - 2, sh.JY - 2]) * zy.B2)) * zx.A2; // ZA2B2 = (Z3B2-(Z23-Z(JX-2,JY-2))*B2)*A2

                // IF (JYML.EQ.0) GO TO 370
                if (jyml == 0) 
                {
                    goto lbl370;
                }

            lbl360:
                zab.ZA2B4 = (zb.Z3B4 - ((z[sh.JX - 2, sh.JY + 1] - zy.Z24) * zy.B4)) * zx.A2; // ZA2B4 = (Z3B4-(Z(JX-2,JY+1)-Z24)*B4)*A2
             
                // IF (JYM2.NE.0) GO TO 390
                if (sh.JYM2 != 0) 
                {
                    goto lbl390;
                }

                zab.ZA2B2 = zab.ZA2B3 + zab.ZA2B3 - zab.ZA2B4; // ZA2B2 = ZA2B3 + ZA2B3 - ZA2B4
                goto lbl390;

            lbl370:
                zab.ZA2B4 = zab.ZA2B3 + zab.ZA2B3 - zab.ZA2B2; // ZA2B4 = ZA2B3 + ZA2B3 - ZA2B2
                goto lbl390;

            lbl380:

                // IF (JXM2.NE.0) GO TO 350
                if (sh.JXM2 != 0) 
                {
                    goto lbl350;
                }

                zab.ZA2B2 = zab.ZA3B2 + zab.ZA3B2 - zab.ZA4B2; // ZA2B2 = ZA3B2 + ZA3B2 - ZA4B2
                zab.ZA2B4 = zab.ZA3B4 + zab.ZA3B4 - zab.ZA4B4; // ZA2B4 = ZA3B4 + ZA3B4 - ZA4B4

                goto lbl420;

            lbl390:
              
                // IF (JXML.NE.0) GO TO 420
                if (jxml != 0) 
                {
                    goto lbl420;
                }

                zab.ZA4B2 = zab.ZA3B2 + zab.ZA3B2 - zab.ZA2B2; // ZA4B2 = ZA3B2 + ZA3B2 - ZA2B2
                zab.ZA4B4 = zab.ZA3B4 + zab.ZA3B4 - zab.ZA2B4; // ZA4B4 = ZA3B4 + ZA3B4 - ZA2B4

                goto lbl420;

            lbl400:
                zab.ZA2B2 = zab.ZA3B2; // ZA2B2 = ZA3B2
                zab.ZA4B2 = zab.ZA3B2; // ZA2B4 = ZA2B3
                zab.ZA2B4 = zab.ZA3B4; // ZA2B4 = ZA3B4
                zab.ZA4B4 = zab.ZA3B4; // ZA4B4 = ZA3B4

                goto lbl420;

            lbl410:
                zab.ZA2B2 = zab.ZA2B3; // ZA2B2 = ZA2B3
                zab.ZA2B4 = zab.ZA2B3; // ZA2B4 = ZA2B3
                zab.ZA4B2 = zab.ZA4B3; // ZA4B2 = ZA4B3
                zab.ZA4B4 = zab.ZA4B3; // ZA4B4 = ZA4B3

            //// NUMERICAL DIFFERENTIATION   ---   TO DETERMINE PARTIAL
            //// DERIVATIVES ZX, ZY, AND ZXY AS WEIGHTED MEANS OF DIVIDED
            //// DIFFERENCES ZA, ZB, AND ZAB, RESPECTIVELY

            lbl420:
              
                // DO 480 JY=2,3
                for (sh.JY = 2; sh.JY <= 3; sh.JY++) 
                {
                    // DO 470 JX=2,3
                    for (sh.JX = 2; sh.JX <= 3; sh.JX++) 
                    {
                        // W2 = ABS(ZA(JX+2,JY-1)-ZA(JX+1,JY-1))
                        sh.W2 = Math.Abs(za[sh.JX + 2, sh.JY - 1] - za[sh.JX + 1, sh.JY - 1]);

                        // W3 = ABS(ZA(JX,JY-1)-ZA(JX-1,JY-1))
                        sh.W3 = Math.Abs(za[sh.JX, sh.JY - 1] - za[sh.JX - 1, sh.JY - 1]);

                        sw = sh.W2 + sh.W3;
                        
                        // IF (SW.EQ.0.0) GO TO 430
                        if (sw < 0.0000001) 
                        {
                            goto lbl430;
                        }

                        zxy.WX2 = sh.W2 / sw;
                        if (float.IsNaN(zxy.WX2))
                        {
                            zxy.WX2 = 0.0f;
                        }

                        zxy.WX3 = sh.W3 / sw;
                        if (float.IsNaN(zxy.WX3))
                        {
                            zxy.WX3 = 0.0f;
                        }

                        goto lbl440;

                    lbl430:
                        zxy.WX2 = 0.5f; // WX2 = 0.5
                        zxy.WX3 = 0.5f; // WX3 = 0.5

                        lbl440:

                        // ZX(JX,JY) = WX2*ZA(JX,JY-1) + WX3*ZA(JX+1,JY-1)
                        zx[sh.JX, sh.JY] = (zxy.WX2 * za[sh.JX, sh.JY - 1]) + (zxy.WX3 * za[sh.JX + 1, sh.JY - 1]);

                        // W2 = ABS(ZB(JX-1,JY+2)-ZB(JX-1,JY+1))
                        sh.W2 = Math.Abs(zb[sh.JX - 1, sh.JY + 2] - zb[sh.JX - 1, sh.JY + 1]);

                        // W3 = ABS(ZB(JX-1,JY)-ZB(JX-1,JY-1))
                        sh.W3 = Math.Abs(zb[sh.JX - 1, sh.JY] - zb[sh.JX - 1, sh.JY - 1]);

                        sw = sh.W2 + sh.W3; // SW = W2 + W3
                      
                        // IF (SW.EQ.0.0) GO TO 450
                        if (sw == 0)
                        {
                            goto lbl450;
                        }

                        sh.WY2 = sh.W2 / sw; ////  WY2 = W2/SW
                        sh.WY3 = sh.W3 / sw; // WY3 = W3/SW

                        goto lbl460;

                    lbl450:
                        sh.WY2 = 0.5f; // WY2 = 0.5
                        sh.WY3 = 0.5f; // WY3 = 0.5

                        lbl460:

                        // ZY(JX,JY) = WY2*ZB(JX-1,JY) + WY3*ZB(JX-1,JY+1)
                        zy[sh.JX, sh.JY] = (sh.WY2 * zb[sh.JX - 1, sh.JY]) + (sh.WY3 * zb[sh.JX - 1, sh.JY + 1]);

                        // ZXY(JX,JY) = WY2*(WX2*ZAB(JX-1,JY-1)+WX3*ZAB(JX,JY-1)) + WY3*(WX2*ZAB(JX-1,JY)+WX3*ZAB(JX,JY))
                        zxy[sh.JX, sh.JY] = (sh.WY2 * ((zxy.WX2 * zab[sh.JX - 1, sh.JY - 1]) + (zxy.WX3 * zab[sh.JX, sh.JY - 1]))) + (sh.WY3 * ((zxy.WX2 * zab[sh.JX - 1, sh.JY]) + (zxy.WX3 * zab[sh.JX, sh.JY])));
                    }
                }

                //// WHEN (U(K).LT.X(1)).OR.(U(K).GT.X(LX))

                // IF (IX.EQ.LXP1) GO TO 530
                if (zxy.IX == zx.LXP1) 
                {
                    goto lbl530;
                }

                // IF (IX.NE.1) GO TO 590
                if (zxy.IX != 1) 
                {
                    goto lbl590;
                }

                sh.W2 = zx.A4 * ((3.0f * a3) + zx.A4); // W2 = A4*(3*A3+A4)
                sh.W1 = (a3 * 2.0f * (a3 - zx.A4)) + sh.W2; // W1 = 2*A3*(A3-A4) + W2

                // DO 500 JY=2,3
                for (sh.JY = 2; sh.JY <= 3; sh.JY++) 
                {
                    // ZX(1,JY) = (W1*ZA(1,JY-1)+W2*ZA(2,JY-1))/(W1+W2)
                    zx[1, sh.JY] = ((sh.W1 * za[1, sh.JY - 1]) + (sh.W2 * za[2, sh.JY - 1])) / (sh.W1 + sh.W2);
                    if (float.IsNaN(zx[1, sh.JY]))
                    {
                        zx[1, sh.JY] = 0.0f;
                    }

                    // ZY(1,JY) = ZY(2,JY) + ZY(2,JY) - ZY(3,JY)
                    zy[1, sh.JY] = zy[2, sh.JY] + zy[2, sh.JY] - zy[3, sh.JY];

                    // ZXY(1,JY) = ZXY(2,JY) + ZXY(2,JY) - ZXY(3,JY)
                    zxy[1, sh.JY] = zxy[2, sh.JY] + zxy[2, sh.JY] - zxy[3, sh.JY];

                    // DO 490 JX1=2,3
                    for (sh.JX1 = 2; sh.JX1 <= 3; sh.JX1++) 
                    {
                        sh.JX = 5 - sh.JX1; // JX = 5 - JX1
                        zx[sh.JX, sh.JY] = zx[sh.JX - 1, sh.JY]; // ZX(JX,JY) = ZX(JX-1,JY)
                        zy[sh.JX, sh.JY] = zy[sh.JX - 1, sh.JY]; // ZY(JX,JY) = ZY(JX-1,JY)
                        zxy[sh.JX, sh.JY] = zxy[sh.JX - 1, sh.JY]; // ZXY(JX,JY) = ZXY(JX-1,JY)
                    }
                }

                x3 = x3 - (1.0f / zx.A4); // X3 = X3 - 1/A4
                sh.Z33 = sh.Z33 - (za.Z3A2 / zx.A4); // Z33 = Z33 - Z3A2/A4
                if (float.IsNaN(sh.Z33))
                {
                    sh.Z33 = 0.0f;
                }

                // DO 510 JY=1,5
                for (sh.JY = 1; sh.JY <= 5; sh.JY++) 
                {
                    // ZB(2,JY) = ZB(1,JY)
                    zb[2, sh.JY] = zb[1, sh.JY]; 
                }

                // DO 520 JY=2,4
                for (sh.JY = 2; sh.JY <= 4; sh.JY++)
                {
                    // ZB(1,JY) = ZB(1,JY) - ZAB(1,JY-1)/A4
                    zb[1, sh.JY] = zb[1, sh.JY] - (zab[1, sh.JY - 1] / zx.A4);
                    if (float.IsNaN(zb[1, sh.JY]))
                    {
                        zb[1, sh.JY] = 0.0f;
                    }
                }

                a3 = zx.A4; // A3 = A4
                sh.JX = 1; // JX = 1
                goto lbl570;

            lbl530:
                sh.W4 = zx.A2 * ((3.0f * a3) + zx.A2); // W4 = A2*(3*A3+A2)
                sh.W5 = (2.0f * a3 * (a3 - zx.A2)) + sh.W4; // W5 = 2*A3*(A3-A2) + W4

                // DO 550 JY=2,3
                for (sh.JY = 2; sh.JY <= 3; sh.JY++) 
                {
                    // ZX(4,JY) = (W4*ZA(4,JY-1)+W5*ZA(5,JY-1))/(W4+W5)
                    zx[4, sh.JY] = ((sh.W4 * za[4, sh.JY - 1]) + (sh.W5 * za[5, sh.JY - 1])) / (sh.W4 + sh.W5);
                    if (float.IsNaN(zx[4, sh.JY]))
                    {
                        zx[4, sh.JY] = 0.0f;
                    }

                    // ZY(4,JY) = ZY(3,JY) + ZY(3,JY) - ZY(2,JY)
                    zy[4, sh.JY] = (zy[3, sh.JY] + zy[3, sh.JY]) - zy[2, sh.JY];

                    // ZXY(4,JY) = ZXY(3,JY) + ZXY(3,JY) - ZXY(2,JY)
                    zxy[4, sh.JY] = (zxy[3, sh.JY] + zxy[3, sh.JY]) - zxy[2, sh.JY];

                    // DO 540 JX=2,3
                    for (sh.JX = 2; sh.JX <= 3; sh.JX++)
                    {
                        zx[sh.JX, sh.JY] = zx[sh.JX + 1, sh.JY]; // ZX(JX,JY) = ZX(JX+1,JY)
                        zy[sh.JX, sh.JY] = zy[sh.JX + 1, sh.JY]; // ZY(JX,JY) = ZY(JX+1,JY)
                        zxy[sh.JX, sh.JY] = zxy[sh.JX + 1, sh.JY]; // ZXY(JX,JY) = ZXY(JX+1,JY)
                    }
                }

                x3 = zx.X4; // X3 = X4
                sh.Z33 = zxy.Z43; // Z33 = Z43

                // DO 560 JY=1,5
                for (sh.JY = 1; sh.JY <= 5; sh.JY++)
                {
                    zb[1, sh.JY] = zb[2, sh.JY]; // ZB(1,JY) = ZB(2,JY)
                }

                a3 = zx.A2; // A3 = A2
                sh.JX = 3; // JX = 3

                lbl570:
                za[3, 1] = za[sh.JX + 1, 1]; // ZA(3,1) = ZA(JX+1,1)

                // DO 580 JY=1,3
                for (sh.JY = 1; sh.JY <= 3; sh.JY++) 
                {
                    // ZAB(2,JY) = ZAB(JX,JY)
                    zab[2, sh.JY] = zab[sh.JX, sh.JY]; 
                }

                //// WHEN (V(K).LT.Y(1)).OR.(V(K).GT.Y(LY))

            lbl590:

                // IF (IY.EQ.LYP1) GO TO 630
                if (zxy.IY == zy.LYP1) 
                {
                    goto lbl630;
                }

                // IF (IY.NE.1) GO TO 680
                if (zxy.IY != 1) 
                {
                    goto lbl680;
                }

                sh.W2 = zy.B4 * ((b3 * 3) + zy.B4); // W2 = B4*(3*B3+B4)
                sh.W1 = (b3 * 2 * (b3 - zy.B4)) + sh.W2; // W1 = 2*B3*(B3-B4) + W2

                // DO 620 JX=2,3
                for (sh.JX = 2; sh.JX <= 3; sh.JX++) 
                {
                    // IF (JX.EQ.3 .AND. IX.EQ.LXP1) GO TO 600
                    // IF (JX.EQ.2 .AND. IX.EQ.1) GO TO 600
                    if ((sh.JX == 3 && zxy.IX == zx.LXP1) || (sh.JX == 2 && zxy.IX == 1))
                    {
                       goto lbl600;
                    }
                    else
                    {
                        //// ZY(JX,1) = (W1*ZB(JX-1,1)+W2*ZB(JX-1,2))/(W1+W2) /// Narendra needs to figure out the NAN issue.
                        zy[sh.JX, 1] = ((sh.W1 * zb[sh.JX - 1, 1]) + (sh.W2 * zb[sh.JX - 1, 2])) / (sh.W1 + sh.W2);

                        if (float.IsNaN(zy[sh.JX, 1]))
                        {
                            zy[sh.JX, 1] = 0.0f;
                        }

                        // ZX(JX,1) = ZX(JX,2) + ZX(JX,2) - ZX(JX,3)
                        zx[sh.JX, 1] = (zx[sh.JX, 2] + zx[sh.JX, 2]) - zx[sh.JX, 3];

                        // ZXY(JX,1) = ZXY(JX,2) + ZXY(JX,2) - ZXY(JX,3)
                        zxy[sh.JX, 1] = (zxy[sh.JX, 2] + zxy[sh.JX, 2]) - zxy[sh.JX, 3];
                    }

                lbl600:

                    // DO 610 JY1=2,3
                    for (sh.JY1 = 2; sh.JY1 <= 3; sh.JY1++) 
                    {
                        sh.JY = 5 - sh.JY1; // JY = 5 - JY1
                        zy[sh.JX, sh.JY] = zy[sh.JX, sh.JY - 1]; // ZY(JX,JY) = ZY(JX,JY-1)
                        zx[sh.JX, sh.JY] = zx[sh.JX, sh.JY - 1]; // ZX(JX,JY) = ZX(JX,JY-1)
                        zxy[sh.JX, sh.JY] = zxy[sh.JX, sh.JY - 1]; // ZXY(JX,JY) = ZXY(JX,JY-1)
                    }
                }

                y3 = y3 - (1.0f / zy.B4); // Y3 = Y3 - 1/B4
                sh.Z33 = sh.Z33 - (zb.Z3B2 / zy.B4); // Z33 = Z33 - Z3B2/B4
                za.Z3A3 = za.Z3A3 - (zab.ZA3B2 / zy.B4); // Z3A3 = Z3A3 - ZA3B2/B4
                zb.Z3B3 = zb.Z3B2; // Z3B3 = Z3B2
                zab.ZA3B3 = zab.ZA3B2; // ZA3B3 = ZA3B2
                b3 = zy.B4; // B3 = B4
                goto lbl670;

            lbl630:
                sh.W4 = zy.B2 * ((3.0f * b3) + zy.B2); // W4 = B2*(3*B3+B2)
                sh.W5 = (2.0f * b3 * (b3 - zy.B2)) + sh.W4; // W5 = 2*B3*(B3-B2) + W4

                // DO 660 JX=2,3
                for (sh.JX = 2; sh.JX <= 3; sh.JX++) 
                {
                    // IF (JX.EQ.3 .AND. IX.EQ.LXP1) GO TO 640
                    // IF (JX.EQ.2 .AND. IX.EQ.1) GO TO 640
                    if ((sh.JX == 3 && zxy.IX == zx.LXP1) || (sh.JX == 2 && zxy.IX == 1))
                    {
                       goto lbl640;
                    }
                    else
                    {
                        // ZY(JX,4) = (W4*ZB(JX-1,4)+W5*ZB(JX-1,5))/(W4+W5)
                        zy[sh.JX, 4] = ((sh.W4 * zb[sh.JX - 1, 4]) + (sh.W5 * zb[sh.JX - 1, 5])) / (sh.W4 + sh.W5);
                        if (float.IsNaN(zy[sh.JX, 4]))
                        {
                            zy[sh.JX, 4] = 0.0f;
                        }

                        // ZX(JX,4) = ZX(JX,3) + ZX(JX,3) - ZX(JX,2)
                        zx[sh.JX, 4] = (zx[sh.JX, 3] + zx[sh.JX, 3]) - zx[sh.JX, 2];

                        // ZXY(JX,4) = ZXY(JX,3) + ZXY(JX,3) - ZXY(JX,2)
                        zxy[sh.JX, 4] = (zxy[sh.JX, 3] + zxy[sh.JX, 3]) - zxy[sh.JX, 2];
                    }

                lbl640:

                    // DO 650 JY=2,3
                    for (sh.JY = 2; sh.JY <= 3; sh.JY++) 
                    {
                       zy[sh.JX, sh.JY] = zy[sh.JX, sh.JY + 1]; // ZY(JX,JY) = ZY(JX,JY+1)
                       zx[sh.JX, sh.JY] = zx[sh.JX, sh.JY + 1]; // ZX(JX,JY) = ZX(JX,JY+1)
                       zxy[sh.JX, sh.JY] = zxy[sh.JX, sh.JY + 1]; // ZXY(JX,JY) = ZXY(JX,JY+1)
                    }
                }

                y3 = zy.Y4; // Y3 = Y4
                sh.Z33 = sh.Z33 + (zb.Z3B3 / b3); // Z33 = Z33 + Z3B3/B3
                za.Z3A3 = za.Z3A3 + (zab.ZA3B3 / b3); // Z3A3 = Z3A3 + ZA3B3/B3
                zb.Z3B3 = zb.Z3B4; // Z3B3 = Z3B4
                zab.ZA3B3 = zab.ZA3B4; // ZA3B3 = ZA3B4
                b3 = zy.B2; // B3 = B2

                lbl670:
               
                // IF (IX.NE.1 .AND. IX.NE.LXP1) GO TO 680
                if (zxy.IX != 1 && zxy.IX != zx.LXP1)
                {
                    goto lbl680;
                }

                sh.JX = (zxy.IX / zx.LXP1) + 2; // JX = IX/LXP1 + 2
                sh.JX1 = 5 - sh.JX; // JX1 = 5 - JX
                sh.JY = (zxy.IY / zy.LYP1) + 2; // JY = IY/LYP1 + 2
                sh.JY1 = 5 - sh.JY; // JY1 = 5 - JY

                // ZX(JX,JY) = ZX(JX1,JY) + ZX(JX,JY1) - ZX(JX1,JY1)
                zx[sh.JX, sh.JY] = (zx[sh.JX1, sh.JY] + zx[sh.JX, sh.JY1]) - zx[sh.JX1, sh.JY1];

                // ZY(JX,JY) = ZY(JX1,JY) + ZY(JX,JY1) - ZY(JX1,JY1)
                zy[sh.JX, sh.JY] = (zy[sh.JX1, sh.JY] + zy[sh.JX, sh.JY1]) - zy[sh.JX1, sh.JY1];

                // ZXY(JX,JY) = ZXY(JX1,JY) + ZXY(JX,JY1) - ZXY(JX1,JY1)
                zxy[sh.JX, sh.JY] = (zxy[sh.JX1, sh.JY] + zxy[sh.JX, sh.JY1]) - zxy[sh.JX1, sh.JY1];

                ////  DETERMINATION OF THE COEFFICIENTS OF THE POLYNOMIAL

            lbl680:
                zx3b3 = (zx.ZX34 - zx.ZX33) * b3;       // ZX3B3 = (ZX34-ZX33)*B3
                zx4b3 = (zx.ZX44 - zx.ZX43) * b3;       // ZX4B3 = (ZX44-ZX43)*B3
                zy3a3 = (zy.ZY43 - zy.ZY33) * a3;       // ZY3A3 = (ZY43-ZY33)*A3
                zy4a3 = (zy.ZY44 - zy.ZY34) * a3;       // ZY4A3 = (ZY44-ZY34)*A3
                zx.A = zab.ZA3B3 - zx3b3 - zy3a3 + zxy.ZXY33;       // A = ZA3B3 - ZX3B3 - ZY3A3 + ZXY33
                zx.B = zx4b3 - zx3b3 - zxy.ZXY43 + zxy.ZXY33;       // B = ZX4B3 - ZX3B3 - ZXY43 + ZXY33
                zx.C = zy4a3 - zy3a3 - zxy.ZXY34 + zxy.ZXY33;       // C = ZY4A3 - ZY3A3 - ZXY34 + ZXY33
                zy.D = zxy.ZXY44 - zxy.ZXY43 - zxy.ZXY34 + zxy.ZXY33;       // D = ZXY44 - ZXY43 - ZXY34 + ZXY33
                zy.E = zx.A + zx.A - zx.B - zx.C;       // E = A + A - B - C
                zx.A3SQ = a3 * a3;      // A3SQ = A3*A3
                zy.B3SQ = b3 * b3;      // B3SQ = B3*B3
                zx.P02 = ((2.0f * (zb.Z3B3 - zy.ZY33)) + zb.Z3B3 - zy.ZY34) * b3;        // P02 = (2*(Z3B3-ZY33)+Z3B3-ZY34)*B3
                zy.P03 = ((-2.0f * zb.Z3B3) + zy.ZY34 + zy.ZY33) * zy.B3SQ;          // P03 = (-2*Z3B3+ZY34+ZY33)*B3SQ
                zy.P12 = ((2.0f * (zx3b3 - zxy.ZXY33)) + zx3b3 - zxy.ZXY34) * b3;        // P12 = (2*(ZX3B3-ZXY33)+ZX3B3-ZXY34)*B3
                zy.P13 = ((-2.0f * zx3b3) + zxy.ZXY34 + zxy.ZXY33) * zy.B3SQ;        // P13 = (-2*ZX3B3+ZXY34+ZXY33)*B3SQ
                zy.P20 = ((2.0f * (za.Z3A3 - zx.ZX33)) + za.Z3A3 - zx.ZX43) * a3;        // P20 = (2*(Z3A3-ZX33)+Z3A3-ZX43)*A3
                zy.P21 = ((2.0f * (zy3a3 - zxy.ZXY33)) + zy3a3 - zxy.ZXY43) * a3;        // P21 = (2*(ZY3A3-ZXY33)+ZY3A3-ZXY43)*A3
                zxy.P22 = ((3.0f * (zx.A + zy.E)) + zy.D) * a3 * b3;     // P22 = (3*(A+E)+D)*A3*B3
                zxy.P23 = ((-3.0f * zy.E) - zx.B - zy.D) * a3 * zy.B3SQ;     // P23 = (-3*E-B-D)*A3*B3SQ
                zxy.P30 = ((-2.0f * za.Z3A3) + zx.ZX43 + zx.ZX33) * zx.A3SQ;     // P30 = (-2*Z3A3+ZX43+ZX33)*A3SQ
                zxy.P31 = ((-2.0f * zy3a3) + zxy.ZXY43 + zxy.ZXY33) * zx.A3SQ;       // P31 = (-2*ZY3A3+ZXY43+ZXY33)*A3SQ
                zxy.P32 = ((-3.0f * zy.E) - zx.C - zy.D) * b3 * zx.A3SQ;     // P32 = (-3*E-C-D)*B3*A3SQ
                zxy.P33 = (zy.D + zy.E + zy.E) * zx.A3SQ * zy.B3SQ;     // P33 = (D+E+E)*A3SQ*B3SQ

                //// COMPUTATION OF THE POLYNOMIAL

            lbl690:
                sh.DY = sh.VK - y3; // DY = VK - Y3
                zx.Q0 = sh.P00 + (sh.DY * (zy.P01 + (sh.DY * (zx.P02 + (sh.DY * zy.P03)))));    // Q0 = P00 + DY*(P01+DY*(P02+DY*P03))
                zx.Q1 = zx.P10 + (sh.DY * (zxy.P11 + (sh.DY * (zy.P12 + (sh.DY * zy.P13)))));   // Q1 = P10 + DY*(P11+DY*(P12+DY*P13))
                zx.Q2 = zy.P20 + (sh.DY * (zy.P21 + (sh.DY * (zxy.P22 + (sh.DY * zxy.P23)))));  // Q2 = P20 + DY*(P21+DY*(P22+DY*P23))
                zy.Q3 = zxy.P30 + (sh.DY * (zxy.P31 + (sh.DY * (zxy.P32 + (sh.DY * zxy.P33)))));    // Q3 = P30 + DY*(P31+DY*(P32+DY*P33))
                sh.DX = sh.UK - x3;     //// DX = UK - X3
                w[k] = zx.Q0 + sh.DX * (zx.Q1 + sh.DX * (zx.Q2 + (sh.DX * zy.Q3)));     //// W(K) = Q0 + DX*(Q1+DX*(Q2+DX*Q3))
            }
        }
    }
}
