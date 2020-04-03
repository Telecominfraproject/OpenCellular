// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.InterpolationLibrary
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents Class CurveCalculator.
    /// </summary>
    public class CurveCalculator
    {
        /// <summary>
        /// Calculates the curve matrix.
        /// </summary>
        /// <param name="freq">The FM or TV frequency, in MHz.</param>
        /// <param name="location">The percent of locations .</param>
        /// <param name="time">The percent of time .</param>
        /// <param name="npoints">The number of points to be calculated for the curve.</param>
        /// <param name="distances">Array of NP distances at which the field is to be calculated in kilometers.</param>
        /// <param name="heights">Array of NP antenna heights above average terrain at which the field is to be calculated in meters.</param>
        /// <param name="fieldStrengths">The field strengths.</param>
        public void CalculateCurveMatrix(float freq, int location, int time, int npoints, FArray distances, FArray heights, FArray fieldStrengths)
        {
            //// The arguments are:                                              
            //// freq -- The FM or TV frequency, in MHz, input, real.            
            //// location - l -> The percent of locations             
            //// time - t -> The percent of time                     
            //// npoints - np -> The number of points to be calculated for the field strength vs distance curve.             
            //// d -- Array of np distances at which the field is to be calculated in kilometers.           
            //// h -- Array of np antenna heights above average terrain ( HAAT ) corresponding to the distance values in array  
            ////      d at which the field is to be calculated in meters.                                         
            //// fs -- output Array of calculated field strength values corresponding to the requested d and h value in db 
            ////       above one microvolt per meter for one kilowatt erp.  
            ////  distance in km & height in meters

            var f5050 = new FArray(201);
            var f5010 = new FArray(201);

            BivariateInterpolation interpolator = new BivariateInterpolation();

            // low freq curves
            if (freq < 108.01f)
            {
                interpolator.Interpolate(25, 13, FCCDataMatrix.D50, FCCDataMatrix.H50, FCCDataMatrix.F55LV, npoints, distances, heights, f5050);
                interpolator.Interpolate(31, 13, FCCDataMatrix.D10, FCCDataMatrix.H10, FCCDataMatrix.F51LV, npoints, distances, heights, f5010);
            }
            else if (freq > 470.0f)
            {
                // UHF curves
                interpolator.Interpolate(25, 13, FCCDataMatrix.D50, FCCDataMatrix.H50, FCCDataMatrix.F55UV, npoints, distances, heights, f5050);
                interpolator.Interpolate(31, 13, FCCDataMatrix.D10, FCCDataMatrix.H10, FCCDataMatrix.F51UV, npoints, distances, heights, f5010);
            }
            else if (freq > 108.0f && freq < 470.0f)
            {
                // VHF curves
                interpolator.Interpolate(25, 13, FCCDataMatrix.D50, FCCDataMatrix.H50, FCCDataMatrix.F55HV, npoints, distances, heights, f5050);
                interpolator.Interpolate(31, 13, FCCDataMatrix.D10, FCCDataMatrix.H10, FCCDataMatrix.F51HV, npoints, distances, heights, f5010);
            }

            // if the distance is less than 15 km copy f5050 value to f5010
            for (int i = 1; i <= npoints; i++)
            {
                if (distances[i] < 15)
                {
                    f5010[i] = f5050[i];
                }
            }

            // AN F(50,50) FIELD STRENGTH CURVE HAS BEEN REQUESTED
            if (location == 50 && time == 50)
            {
                for (int i = 1; i <= npoints; i++)
                {
                    fieldStrengths[i] = f5050[i];
                }
            }
            else if (location == 50 && time == 10)
            {
                // AN F(50,10) FIELD STRENGTH CURVE HAS BEEN REQUESTED
                for (int i = 1; i <= npoints; i++)
                {
                    fieldStrengths[i] = f5010[i];
                }
            }
            else if (location == 50 && time == 90)
            {
                // AN F(50,90) FIELD STRENGTH CURVE HAS BEEN REQUESTED
                float zq = 0;

                this.FZQ(location, ref zq);

                float sigma = 8.58f;

                if (freq > 470)
                {
                    sigma = 11.88f;
                }

                float rl = zq * sigma;
                float rt = 0;

                this.FZQ(time, ref zq);

                for (int i = 1; i <= npoints; i++)
                {
                    rt = (f5010[i] - f5050[i]) * zq / 1.28155f;
                    fieldStrengths[i] = (float)(f5050[i] + rl + rt);
                }
            }
        }

        /// <summary>
        /// Calculate quality from time or location.
        /// </summary>
        /// <param name="q">The location or time (l or t).</param>
        /// <param name="zq">The quality factor.</param>
        public void FZQ(float q, ref float zq)
        {
            FArray zgrid = new FArray(115);

            for (int i = 1; i <= 57; i++)
            {
                zgrid[i] = -FCCDataMatrix.ZGRI[i];
                zgrid[i + 58] = FCCDataMatrix.ZGRI[58 - i];
            }

            zgrid[58] = FCCDataMatrix.ZGRI[58];

            for (int i = 1; i <= 115; i++)
            {
                if (FCCDataMatrix.VGRID[i] < q)
                {
                    continue;
                }

                // found point calculate zq and return 
                float perc = (q - FCCDataMatrix.VGRID[i - 1]) / (FCCDataMatrix.VGRID[i] - FCCDataMatrix.VGRID[i - 1]);
                zq = zgrid[i - 1] + (perc * (zgrid[i] - zgrid[i - 1]));

                return;
            }
        }

        /// <summary>
        /// Calculates the curve value.
        /// </summary>
        /// <param name="erpx">ERP in kW.</param>
        /// <param name="haatx">HAAT in meters.</param>
        /// <param name="ichannel">The channel no.</param>
        /// <param name="field">field strength in dBu.</param>
        /// <param name="distance">distance to the contour in km.</param>
        /// <param name="ichoise">choice 1-&gt; field strength, given distance, 2 -&gt; distance, given field strength.</param>
        /// <param name="isDigital">if set to <c>true</c> [is digital].</param>
        /// <returns>returns field strength or distance.</returns>
        public float CalculateCurveValue(float erpx, float haatx, int ichannel, float field, float distance, int ichoise, bool isDigital)
        {
            //// ichannel --- input channel
            //// ichoise  --- 1 = field strength, given distance;
            ////              2 = distance, given field strength.
            //// erpx -------- proposed erp in kW
            //// haatx ------   proposed haat in meters
            //// field ---   field strength in dBu
            //// distance ----  distance to the contour in km.

            FArray fs = new FArray(201);
            FArray d = new FArray(201);
            FArray h = new FArray(201);

            float range = 100.0f;
            int n_points = 201;
            float freq = 0.0f;

            if (ichannel >= 2 && ichannel <= 6)
            {
                freq = 100.0F; // channels 2-6
            }
            else if (ichannel >= 7 && ichannel <= 13)
            {
                freq = 180.1F; // channels 7-13
            }
            else if (ichannel >= 14)
            {
                freq = 480.0F; // channels 14 and above
            }

            float erp_db = (float)(10 * Math.Log10(erpx));

            if (ichoise == 1)
            {
                // find field strength, given distance
                d[1] = (float)distance;
                h[1] = (float)haatx;

                if (isDigital)
                {
                    this.CalculateCurveMatrix(freq, 50, 90, 1, d, h, fs);
                }
                else
                {
                    this.CalculateCurveMatrix(freq, 50, 50, 1, d, h, fs);
                }

                field = erp_db + fs[1];

                return field;
            }
            else
            {
                float d_first = 1.5f; // start at 1.5 kM
                float d_last = 300.0f; // end at 300 kM

                float elevHeight = 0.0f;

                if (haatx < 30)
                {
                    elevHeight = 30;
                }
                else if (haatx > 1600)
                {
                    elevHeight = 1600;
                }
                else
                {
                    elevHeight = haatx;
                }

                for (int kk = 1; kk <= n_points; kk++)
                {
                    // increments of 0.5 km
                    d[kk] = (float)(d_first + ((kk - 1) * 0.5));
                    h[kk] = (float)elevHeight;
                }

            lbl510:
                if (isDigital)
                {
                    this.CalculateCurveMatrix(freq, 50, 90, n_points, d, h, fs);
                }
                else
                {
                    this.CalculateCurveMatrix(freq, 50, 50, n_points, d, h, fs);
                }

                for (int kk = 1; kk <= n_points; kk++)
                {
                    fs[kk] = (float)(fs[kk] + erp_db);
                    ////Console.WriteLine("Index :- " + kk + " :: " + fs[kk]);
                }

                // check the first point
                if (field > fs[1])
                {
                    // free space equation
                    float e_volts_meter = (float)(1.0E-6 * Math.Pow(10.0, field / 20.0));

                    if (isDigital)
                    {
                        distance = (float)(0.007014271F * Math.Sqrt(erpx * 1000));
                    }
                    else
                    {
                        distance = (float)(0.007014271F * Math.Sqrt(erpx * 1000)) / e_volts_meter;
                        if (distance > 1.5)
                        {
                            distance = 1.5F;
                        }
                    }
                }
                else if (field < fs[n_points])
                {
                    // check the last point
                    for (int i = 1; i <= n_points; i++)
                    {
                        d[i] = d[i] + range;
                    }

                    if (d[1] < d_last)
                    {
                        goto lbl510;
                    }
                    else
                    {
                        // if distance is greater than last curve value then set it to last curve value
                        distance = d_last;
                    }
                }
                else
                {
                    // field value lies within the range
                    for (int i = 2; i <= n_points; i++)
                    {
                        if (field <= fs[i - 1] && field >= fs[i])
                        {
                            distance = ((field - fs[i - 1]) / (fs[i] - fs[i - 1]) * (d[i] - d[i - 1])) + d[i - 1];

                            if (distance > d_last)
                            {
                                // if distance is greater than last curve value then set it to last curve value
                                distance = d_last;
                            }

                            break;
                        }
                    }
                }

                return distance;
            }
        }
    }
}
