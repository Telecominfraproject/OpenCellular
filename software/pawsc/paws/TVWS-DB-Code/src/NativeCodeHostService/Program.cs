// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace NativeCodeHostService
{
    using System;
    using System.Linq;
    using System.ServiceModel;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Description;

    internal class Program
    {
        private static void Main(string[] args)
        {
            Uri address = new Uri("net.pipe://localhost/NativeCodeService");
            NetNamedPipeBinding binding = new NetNamedPipeBinding();
            binding.ReceiveTimeout = TimeSpan.MaxValue;
            binding.SendTimeout = TimeSpan.MaxValue;
            ////binding.MaxReceivedMessageSize = 40000000;
            ////binding.ReaderQuotas.MaxArrayLength = int.MaxValue;
            ////binding.ReaderQuotas.MaxStringContentLength = int.MaxValue;

            using (ServiceHost host = new ServiceHost(typeof(NativeMethods)))
            {
                var ff = host.AddServiceEndpoint(typeof(INativeMethodService), binding, address);
                ServiceMetadataBehavior metadata = new ServiceMetadataBehavior();

                host.Description.Behaviors.Add(metadata);
                host.Description.Behaviors.OfType<ServiceDebugBehavior>().First().IncludeExceptionDetailInFaults = true;

                Binding mexBinding = MetadataExchangeBindings.CreateMexNamedPipeBinding();
                Uri mexAddress = new Uri("net.pipe://localhost/NativeCodeService/Mex");
                host.AddServiceEndpoint(typeof(IMetadataExchange), mexBinding, mexAddress);

                host.Open();

                Console.WriteLine("The Native Code Host Service is ready");
                Console.ReadLine();
            }
        }
    }
}
