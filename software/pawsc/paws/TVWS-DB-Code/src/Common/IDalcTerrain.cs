// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents the Terrain interface into the data access layer component.
    /// </summary>
    public interface IDalcTerrain
    {
        /// <summary>
        /// Loads the specified terrain file from the storage.
        /// </summary>
        /// <param name="fileName">File name that is to be retrieved from storage.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <param name="terrainElementSize">Size of the terrain element.</param>
        /// <returns>File data.</returns>
        object LoadTerrainFile(string fileName, string dataDirectory, int terrainElementSize);

        /// <summary>
        /// Checks the terrain file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool CheckTerrainFileExists(string fileName, string dataDirectory);

        /// <summary>
        /// Loads the terrain file.
        /// </summary>
        /// <param name="fileName">Name of the file.</param>
        /// <param name="dataDirectory">The data directory.</param>
        /// <returns>returns Stream.</returns>
        Stream LoadTerrainFile(string fileName, string dataDirectory);
    }
}
