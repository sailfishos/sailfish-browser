from django.conf.urls import patterns, include, url

# Uncomment the next two lines to enable the admin:
# from django.contrib import admin
# admin.autodiscover()

urlpatterns = patterns('',
    # Examples:
    url(r'^$', 'testapp.views.home', name='home'),
    # url(r'^webserver/', include('webserver.foo.urls')),
    url(r'^auth$', 'testapp.views.testauth', name='testauth'),
    url(r'^upload/', 'testapp.views.testupload', name='testupload'),
    url(r'^ajaxpage/$', 'testapp.views.show_ajax_page', name='show_ajax_page'),
    url(r'^testajaxreq/$', 'testapp.views.testajaxreq', name='testajaxreq'),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^admin/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
    # url(r'^admin/', include(admin.site.urls)),
)
