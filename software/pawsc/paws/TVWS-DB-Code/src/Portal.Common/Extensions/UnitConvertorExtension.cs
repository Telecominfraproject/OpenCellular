// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.UnitConvertor
{
    using System;
    using Microsoft.Whitespace.Entities;

    public static class UnitConvertorExtension
    {
        private const double FeetToMeterConversionFactor = 0.3048;

        private const double MeterToKiloMeterConverstionFactor = 0.001;

        private const double KiloMeterToMileConverstionFactor = 0.62137;

        public static double ToPower(this double value, PowerUnit sourceUnit, PowerUnit targetUnit)
        {
            switch (sourceUnit)
            {
                case PowerUnit.dB:
                    return value.ConvertdBTo(targetUnit);
                case PowerUnit.Watt:
                    return value.ConvertWattTo(targetUnit);
                case PowerUnit.KiloWatt:
                    return value.ConvertKiloWattTo(targetUnit);
                case PowerUnit.MegaWatt:
                    return value.ConvertMegaWattTo(targetUnit);
            }

            throw new NotSupportedException("sourceUnit");
        }

        public static double ToDistance(this double value, DistanceUnit sourceUnit, DistanceUnit targetUnit)
        {
            switch (sourceUnit)
            {
                case DistanceUnit.Feet:
                    return value.ConvertFootTo(targetUnit);
                case DistanceUnit.Meter:
                    return value.ConvertMeterTo(targetUnit);
                case DistanceUnit.KM:
                    return value.ConvertKiloMeterTo(targetUnit);
                case DistanceUnit.Miles:
                    return value.ConvertMileTo(targetUnit);
            }

            throw new NotSupportedException("sourceUnit");
        }

        private static double ConvertdBTo(this double value, PowerUnit targetUnit)
        {
            switch (targetUnit)
            {
                case PowerUnit.Watt:
                    return Math.Pow(10, value - (30 / 10));
                case PowerUnit.KiloWatt:
                    return Math.Pow(10, value - (60 / 10));
                case PowerUnit.MegaWatt:
                    return Math.Pow(10, value - (90 / 10));
            }

            throw new NotSupportedException("targetUnit");
        }

        private static double ConvertWattTo(this double value, PowerUnit targetUnit)
        {
            switch (targetUnit)
            {
                case PowerUnit.dB:
                    return 10 * Math.Log10(1000 * value);
                case PowerUnit.KiloWatt:
                    return value / 1000;
                case PowerUnit.MegaWatt:
                    return value / 1000000;
            }

            throw new NotSupportedException("targetUnit");
        }

        private static double ConvertKiloWattTo(this double value, PowerUnit targetUnit)
        {
            switch (targetUnit)
            {
                case PowerUnit.dB:
                    return 10 * Math.Log10(1000000 * value);
                case PowerUnit.Watt:
                    return value * 1000;
                case PowerUnit.MegaWatt:
                    return value / 1000;
            }

            throw new NotSupportedException("targetUnit");
        }

        private static double ConvertMegaWattTo(this double value, PowerUnit targetUnit)
        {
            switch (targetUnit)
            {
                case PowerUnit.dB:
                    return 10 * Math.Log10(1000000000 * value);
                case PowerUnit.Watt:
                    return value * 1000000;
                case PowerUnit.KiloWatt:
                    return value * 1000;
            }

            throw new NotSupportedException("targetUnit");
        }

        private static double ConvertMeterTo(this double value, DistanceUnit targetUnit)
        {
            switch (targetUnit)
            {
                case DistanceUnit.Feet:
                    return value / FeetToMeterConversionFactor;
                case DistanceUnit.KM:
                    return value * MeterToKiloMeterConverstionFactor;
                case DistanceUnit.Miles:
                    return value * MeterToKiloMeterConverstionFactor * KiloMeterToMileConverstionFactor;
            }

            throw new NotSupportedException("targetUnit");
        }

        private static double ConvertFootTo(this double value, DistanceUnit targetUnit)
        {
            switch (targetUnit)
            {
                case DistanceUnit.Meter:
                    return value * FeetToMeterConversionFactor;
                case DistanceUnit.KM:
                    return value * FeetToMeterConversionFactor * MeterToKiloMeterConverstionFactor;
                case DistanceUnit.Miles:
                    return value * FeetToMeterConversionFactor * MeterToKiloMeterConverstionFactor * KiloMeterToMileConverstionFactor;
            }

            throw new NotSupportedException("targetUnit");
        }

        private static double ConvertKiloMeterTo(this double value, DistanceUnit targetUnit)
        {
            switch (targetUnit)
            {
                case DistanceUnit.Feet:
                    return (value / MeterToKiloMeterConverstionFactor) / FeetToMeterConversionFactor;
                case DistanceUnit.Meter:
                    return value / MeterToKiloMeterConverstionFactor;
                case DistanceUnit.Miles:
                    return value * KiloMeterToMileConverstionFactor;
            }

            throw new NotSupportedException("targetUnit");
        }

        private static double ConvertMileTo(this double value, DistanceUnit targetUnit)
        {
            switch (targetUnit)
            {
                case DistanceUnit.Feet:
                    return ((value / KiloMeterToMileConverstionFactor) / MeterToKiloMeterConverstionFactor) / FeetToMeterConversionFactor;
                case DistanceUnit.Meter:
                    return (value / KiloMeterToMileConverstionFactor) / MeterToKiloMeterConverstionFactor;
                case DistanceUnit.KM:
                    return value * KiloMeterToMileConverstionFactor;
            }

            throw new NotSupportedException("targetUnit");
        }
    }
}
