from django.urls import include, path
from rest_framework import routers
from base.src import views
from django.conf.urls import url, include



# Wire up our API using automatic URL routing.
# Additionally, we include login URLs for the browsable API.
urlpatterns = [
    path('api-auth/', include('rest_framework.urls', namespace='rest_framework')),
    url(r'^api/', include('base.src.urls'))
]