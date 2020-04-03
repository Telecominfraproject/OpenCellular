// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Test.Unit
{
    using System.Web.Mvc;
    using Microsoft.WhiteSpaces.BusinessManager.Fakes;
    using Microsoft.WhiteSpaces.Common.Fakes;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WhiteSpaces.Portal;

    /// <summary>
    /// Test cases for HomeController methods
    /// </summary>
    [TestClass]
    public class HomeControllerTests
    {
        /// <summary>
        /// variable to hold <see cref="HomeController"/>
        /// </summary>
        private static HomeController homeController;

        /// <summary>
        /// Class initialize method
        /// </summary>
        /// <param name="context">Test Context</param>
        [ClassInitialize]
        public static void Initialize(TestContext context)
        {
            StubIWhitespacesManager whitespaceManager = new StubIWhitespacesManager();
            StubIRegionSource regionSource = new StubIRegionSource();

            homeController = new HomeController(whitespaceManager, regionSource);
        }

        /// <summary>
        /// Test Index view method
        /// </summary>
        [TestMethod]
        public void Index_view_result_should_not_be_null()
        {
            var result = homeController.Index() as ViewResult;
            Assert.IsNotNull(result);
        }

        /// <summary>
        /// Test TermsOfUse view method
        /// </summary>
        [TestMethod]
        public void TermsOfUse_view_result_should_not_be_null()
        {
            var result = homeController.TermsOfUse() as ViewResult;
            Assert.IsNotNull(result);
        }

        /// <summary>
        /// Test ContactUs view method
        /// </summary>
        [TestMethod]
        public void ContactUs_view_result_should_not_be_null()
        {
            var result = homeController.ContactUs() as ViewResult;
            Assert.IsNotNull(result);
        }

        /// <summary>
        /// Test About view method
        /// </summary>
        [TestMethod]
        public void About_view_result_should_not_be_null()
        {
            var result = homeController.About() as ViewResult;
            Assert.IsNotNull(result);
        }

        /// <summary>
        /// Test PrivacyStatement view method
        /// </summary>
        [TestMethod]
        public void PrivacyStatement_view_result_should_not_be_null()
        {
            var result = homeController.Privacy() as ViewResult;
            Assert.IsNotNull(result);
        }
    }
}
