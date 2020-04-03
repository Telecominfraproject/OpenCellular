// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;

    public class BlobOperations
    {
        public readonly CloudStorageAccount StorageAccount;

        public readonly CloudBlobClient BlobClient;

        public readonly CloudBlobContainer BlobContainer;

        public BlobOperations()
        {
            // Retrieve storage account from connection string.
            this.StorageAccount = CloudStorageAccount.Parse(this.StorageConnectionString);

            // Create the blob client.
            this.BlobClient = this.StorageAccount.CreateCloudBlobClient();

            this.BlobContainer = this.BlobClient.GetContainerReference(this.Container);
            this.BlobContainer.CreateIfNotExists();
            this.BlobContainer.SetPermissions(
                new BlobContainerPermissions
                {
                    PublicAccess =
                        BlobContainerPublicAccessType.Blob
                });
        }

        public string StorageConnectionString
        {
            get
            {
                return RoleEnvironment.IsAvailable
                            ? RoleEnvironment.GetConfigurationSettingValue("AzureStorageAccountConnectionString")
                            : ConfigurationManager.ConnectionStrings["AzureStorageAccountConnectionString"].ConnectionString;
            }
        }

        public string Container
        {
            get
            {
                return "testresults";
            }
        }

        public string[] UploadFilesandGetUrls(Dictionary<string, System.IO.Stream> blobStreams)
        {
            List<string> blobFiles = new List<string>();

            // Save to blobs
            foreach (var blobStream in blobStreams)
            {
                CloudBlockBlob blockBlob = this.BlobContainer.GetBlockBlobReference(blobStream.Key);
                blockBlob.UploadFromStream(blobStream.Value);
            }

            // Get Urls of blobs
            foreach (var blobStream in blobStreams)
            {
                foreach (IListBlobItem item in this.BlobContainer.ListBlobs(blobStream.Key, false))
                {
                    if (item.GetType() == typeof(CloudBlockBlob))
                    {
                        CloudBlockBlob blob = (CloudBlockBlob)item;

                        blobFiles.Add(blob.Uri.ToString());
                    }
                }
            }

            return blobFiles.ToArray();
        }
    }
}
