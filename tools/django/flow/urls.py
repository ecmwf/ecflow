
from django.conf.urls import include, url
from django.views.generic import ListView, DetailView
from django.contrib import admin
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.views.generic.base import RedirectView
from . import views

admin.autodiscover()
urlpatterns = [ # 'fdi.views', # Examples:
    url(r'^$', views.home),
    url(r'^api/ecflow', views.process),
    url(r'^api/sms', views.process),
    url(r'^slide2', views.slide2),
    url(r'^slide3', views.slide3),
    url(r'^triggers', views.triggers),
    url(r'^json', views.tojson),
    url(r'^edge$', views.edge),
    url(r'^vars$', views.vars),
    url(r'^admin/doc/', include('django.contrib.admindocs.urls')),
    url(r'^admin/', include(admin.site.urls)),
]

urlpatterns += staticfiles_urlpatterns()

# if fdi.settings.DEBUG:
#     import debug_toolbar
#     urlpatterns += patterns('',
#         url(r'^__debug__/', include(debug_toolbar.urls)), )

REST_FRAMEWORK = { 'DEFAULT_PERMISSION_CLASSES': ['rest_framework.permissions.DjangoModelPermissionsOrAnonReadOnly' ]    }
# LOGIN_REDIRECT_URL = "sst"

