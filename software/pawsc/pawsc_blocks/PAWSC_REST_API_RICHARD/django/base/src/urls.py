# create this file
# rerouting all requests that have ‘api’ in the url to the <code>apps.core.urls
from django.conf.urls import url
from django.urls import path
from rest_framework import routers
from base.src import views

from base.src.views import  InitViewSet
#from base.src.views import UploadFileForm

#upload stuff
from django.conf import settings
from django.conf.urls.static import static

router = routers.DefaultRouter()

router.register(r'users', views.UserViewSet)
router.register(r'groups', views.GroupViewSet)

#router.register(r'titles', TitlesViewSet, base_name='titles')

urlpatterns = [
    path(r'pawsc', InitViewSet.as_view()),
    path(r'pawsc/upload', views.simple_upload, name='simple_upload'),
    path(r'pawsc/reports', views.spectrum_analysis_report, name='spectrum_measurement_reports'),
    path(r'pawsc/home', views.home, name='home')
     # path(r'pawsc/home', views.model_form_upload, name='model_form_upload')
  
] + static(settings.STATIC_URL, document_root=settings.STATIC_ROOT)   
 
urlpatterns += router.urls
