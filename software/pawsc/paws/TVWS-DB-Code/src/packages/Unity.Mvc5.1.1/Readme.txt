Getting started with Unity.Mvc5
-------------------------------

Unity.Mvc5 is an update of the popular Unity.Mvc3 package, updated to target .NET 4.5, MVC5 and Unity 3.0

To get started, just add a call to UnityConfig.RegisterComponents() in the Application_Start method of Global.asax.cs 
and the MVC framework will then use the Unity.Mvc5 DependencyResolver to resolve your components.

e.g.
 
public class MvcApplication : System.Web.HttpApplication
{
  protected void Application_Start()
  {
    AreaRegistration.RegisterAllAreas();
    UnityConfig.RegisterComponents();                           // <----- Add this line
    FilterConfig.RegisterGlobalFilters(GlobalFilters.Filters);
    RouteConfig.RegisterRoutes(RouteTable.Routes);
    BundleConfig.RegisterBundles(BundleTable.Bundles);
  }           
}  

Add your Unity registrations in the RegisterComponents method of the UnityConfig class. All components that implement IDisposable should be 
registered with the HierarchicalLifetimeManager to ensure that they are properly disposed at the end of the request.

It is not necessary to register your controllers with Unity.