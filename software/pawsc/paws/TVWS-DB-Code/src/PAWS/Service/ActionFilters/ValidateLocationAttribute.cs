// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.PAWS.Service.ActionFilters
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Net.Http;
    using System.Web.Http.Controllers;
    using System.Web.Http.Filters;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.PAWS.Service.Utilities;

    public class ValidateLocationAttribute : ActionFilterAttribute
    {
        private const string RequestArgumentName = "httpRequest";

        /// <summary>
        /// Gets or sets IAuditor Interface
        /// </summary>
        [Dependency]
        public IAuditor PawsAuditor { get; set; }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        [Dependency]
        public ILogger PawsLogger { get; set; }

        public override void OnActionExecuting(HttpActionContext actionContext)
        {
            Stopwatch stopWatch = new Stopwatch();

            stopWatch.Start();

            var request = actionContext.ActionArguments[RequestArgumentName];
            bool enableValidation = false;

            if (Utils.Configuration.HasSetting(Constants.ValidateWSDLocation))
            {
                enableValidation = Utils.Configuration[Constants.ValidateWSDLocation].ToBool();
            }

            if (request == null || !enableValidation)
            {
                return;
            }

            HttpRequestMessage httpRequest = (HttpRequestMessage)request;
            Request requestparam = new Request();

            this.PawsAuditor.UserId = this.PawsLogger.UserId;
            this.PawsAuditor.TransactionId = this.PawsLogger.TransactionId;
            this.PawsAuditor.RegionCode = Utils.Configuration.CurrentRegionId;

            try
            {
                requestparam = JsonSerialization.DeserializeString<Request>(httpRequest.Content.ReadAsStringAsync().Result);
                GeoLocation geolocation = requestparam.Params.Location;
                GeoLocation[] geolocations = requestparam.Params.Locations;

                List<RegionPolygonsCache> subregions = (List<RegionPolygonsCache>)DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.RegionPolygons, null);

                bool isValidRequest = false;
                bool isBatchRequest = false;

                if (geolocation != null)
                {
                    isValidRequest = ValidateRequestedLocation(geolocation, subregions);
                }
                else if (geolocations != null && geolocations.Length > 0)
                {
                    isBatchRequest = true;
                    isValidRequest = ValidateBatchRequestLocations(geolocations, subregions);
                }

                stopWatch.Stop();

                if (!isValidRequest)
                {
                    PawsResponse errorResponse = null;

                    if (isBatchRequest)
                    {
                        errorResponse = ErrorHelper.CreateErrorResponse(requestparam.Method, geolocations, Constants.ErrorMessageOutsideCoverage);
                    }
                    else
                    {
                        errorResponse = ErrorHelper.CreateErrorResponse(requestparam.Method, geolocation, Constants.ErrorMessageOutsideCoverage);
                    }

                    this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, errorResponse.Error.Message);

                    string auditMethod;
                    AuditId auditId = PawsUtil.GetAuditId(requestparam.Method, out auditMethod);

                    this.PawsAuditor.Audit(auditId, AuditStatus.Failure, stopWatch.ElapsedMilliseconds, auditMethod + " failed");

                    actionContext.Response = actionContext.Request.CreateResponse<PawsResponse>(errorResponse);
                }
            }
            catch (Exception ex)
            {
                stopWatch.Stop();

                this.PawsLogger.Log(TraceEventType.Error, LoggingMessageId.PAWSGenericMessage, ex.ToString());

                string auditMethod;
                AuditId auditId = PawsUtil.GetAuditId(requestparam.Method, out auditMethod);

                this.PawsAuditor.Audit(auditId, AuditStatus.Failure, stopWatch.ElapsedMilliseconds, auditMethod + " failed");

                PawsResponse errorResponse = ErrorHelper.CreateExceptionResponse(requestparam.Method, ex.Message);

                actionContext.Response = actionContext.Request.CreateResponse<PawsResponse>(errorResponse);
            }
        }

        private static bool ValidateBatchRequestLocations(GeoLocation[] geolocations, List<RegionPolygonsCache> subregions)
        {
            bool anyValidLocation = false;

            foreach (GeoLocation geolocation in geolocations)
            {
                anyValidLocation = ValidateRequestedLocation(geolocation, subregions);

                // [SPECTRUM_USE_RESP (Section 4.5.6)] If some, but not all, locations within a batch request are outside the regulatory domain supported by 
                // the Database, the Database must return OK response.
                if (anyValidLocation)
                {
                    return true;
                }
            }

            return anyValidLocation;
        }

        private static bool ValidateRequestedLocation(GeoLocation geolocation, List<RegionPolygonsCache> subregions)
        {
            List<Point> points = new List<Point>();

            if (geolocation.Point != null)
            {
                points.Add(geolocation.Point.Center);
            }
            else if (geolocation.Region != null)
            {
                points.AddRange(geolocation.Region.Exterior);
            }

            bool isValidLocation = true;

            foreach (Point wsdLocation in points)
            {
                double latitude = wsdLocation.Latitude.ToDouble();
                double longitude = wsdLocation.Longitude.ToDouble();

                isValidLocation = GeoCalculations.IsPointInRegionPolygons(subregions, latitude, longitude);

                if (!isValidLocation)
                {
                    break;
                }
            }

            return isValidLocation;
        }
    }
}
