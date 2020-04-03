# create this file
# rerouting all requests that have ‘api’ in the url to the <code>apps.core.urls
from django.conf.urls import url
from django.urls import path
from rest_framework import routers
from base.src import views

from base.src.views import  InitViewSet

router = routers.DefaultRouter()

router.register(r'users', views.UserViewSet)
router.register(r'groups', views.GroupViewSet)

#router.register(r'titles', TitlesViewSet, base_name='titles')

urlpatterns = [
    path(r'pawsc', InitViewSet.as_view())
]   
 
urlpatterns += router.urls