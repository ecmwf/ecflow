# Django settings for ecflow project.
import os
import pwd
import pylzma
import platform
from os.path import dirname, join, realpath

PROJECT_DIR = dirname(__file__)
DEBUG = True
# TEMPLATE_DEBUG = DEBUG
USER = pwd.getpwuid(os.getuid())[0]
ADMINS = (
    (USER, "%s@localhost" % USER),
)

MANAGERS = ADMINS
GRAPH_MODELS = {
  'all_applications': True,
  'group_models': True,
}

DATABASES = {
    'default': {
'ENGINE' : 'django.db.backends.sqlite3', # Add 'postgresql_psycopg2', 'mysql', 'sqlite3' 
        'NAME': join(PROJECT_DIR, '%s.sql' % USER),# Or path to database file if using sqlite3.
        'USER': '',                      # Not used with sqlite3.
        'PASSWORD': '',                  # Not used with sqlite3.
        'HOST': '',                      # Set to empty string for localhost. Not used with sqlite3.
        'PORT': '',                      # Set to empty string for default. Not used with sqlite3.
    },
}

TIME_ZONE = 'Europe/London'

LANGUAGE_CODE = 'en-uk'

SITE_ID = 1

USE_I18N = True

USE_L10N = True

USE_TZ = True
TOPDIR = dirname(realpath(__file__))
# import platform; if platform.node() == "bilbo": pass

# Absolute filesystem path to the directory that will hold user-uploaded files.
# Example: "/home/media/media.lawrence.com/media/"
MEDIA_ROOT = TOPDIR + "media/"

# URL that handles the media served from MEDIA_ROOT. Make sure to use a
# trailing slash.
# Examples: "http://media.lawrence.com/media/", "http://example.com/media/"
MEDIA_URL = ''

# Absolute path to the directory static files should be collected to.
# Don't put anything in this directory yourself; store your static files
# in apps' "static/" subdirectories and in STATICFILES_DIRS.
# Example: "/home/media/media.lawrence.com/static/"
STATIC_ROOT = TOPDIR + 'static/'

# URL prefix for static files.
# Example: "http://media.lawrence.com/static/"
STATIC_URL = '/static/'

# Additional locations of static files
STATICFILES_DIRS = (
    # Put strings here, like "/home/html/static" or "C:/www/django/static".
    # Always use forward slashes, even on Windows.
    # Don't forget to use absolute paths, not relative paths.
)

# List of finder classes that know how to find static files in
# various locations.
STATICFILES_FINDERS = (
    'django.contrib.staticfiles.finders.FileSystemFinder',
    'django.contrib.staticfiles.finders.AppDirectoriesFinder',
#    'django.contrib.staticfiles.finders.DefaultStorageFinder',
)

# Make this unique, and don't share it with anybody.
SECRET_KEY = 'je5fyp0d(_1nc_*r(j$oxt0(wpy2a-p-l5jy^6#4qld79=2fc$'

MIDDLEWARE_CLASSES = (
    'django.middleware.common.CommonMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
    # Uncomment the next line for simple clickjacking protection:
    # 'django.middleware.clickjacking.XFrameOptionsMiddleware',
)

ROOT_URLCONF = 'flow.urls'
# Python dotted path to the WSGI application used by Django's runserver.
WSGI_APPLICATION = 'flow.wsgi.application'
TEMPLATES = [ {
    'BACKEND': 'django.template.backends.django.DjangoTemplates',
    'DIRS': [ TOPDIR + "/template", 
              # TOPDIR + "/flow/template", 
          ],
#     'AUTH': ' django.contrib.auth.context_processors.auth',
    'APP_DIRS': True,
    'OPTIONS': {
        'context_processors': [
            'django.template.context_processors.debug',
            'django.template.context_processors.request',
            'django.contrib.auth.context_processors.auth',
            'django.contrib.messages.context_processors.messages',
        ],},
    },] 

#TEMPLATE_DIRS = (
#    TOPDIR + "/template",
    # TOPDIR + "/flow/template",
    # TOPDIR + "flow/templatetags",
#)

INSTALLED_APPS = (
    'django.contrib.auth',
    'django.contrib.contenttypes',
    'django.contrib.sessions',
    'django.contrib.sites',
    'django.contrib.messages',
    'django.contrib.staticfiles',
    'bootstrap3',
    # Uncomment the next line to enable the admin:
    'django.contrib.admin',
    # Uncomment the next line to enable admin documentation:
    'django.contrib.admindocs',

    'flow',
    # 'flow.templatetags',

    # 'django_tasks',
    # 'djsupervisor',
    'rest_framework',

    # 'ds', 
    # 'sst',
    # 'sms',

    'django_extensions',
)

# DEBUG_TOOLBAR_PANELS = (
#     'debug_toolbar_mongo.panel.MongoDebugPanel',
# )
# A sample logging configuration. The only tangible logging
# performed by this configuration is to send an email to
# the site admins on every HTTP 500 error when DEBUG=False.
# See http://docs.djangoproject.com/en/dev/topics/logging for
# more details on how to customize your logging configuration.
LOGGING = {
    'version': 1,
    'disable_existing_loggers': False,
    'filters': {
        'require_debug_false': {
            '()': 'django.utils.log.RequireDebugFalse'
        }
    },
    'handlers': {
        'mail_admins': {
            'level': 'ERROR',
            'filters': ['require_debug_false'],
            'class': 'django.utils.log.AdminEmailHandler'
        }
    },
    'loggers': {
        'django.request': {
            'handlers': ['mail_admins'],
            'level': 'ERROR',
            'propagate': True,
        },
    }
}

DJANGOTASK_DAEMON_THREAD = True
CACHES = {
    "default": {
        "BACKEND": "django_redis.cache.RedisCache",
        "LOCATION": "redis://127.0.0.1:6379/1",
        "OPTIONS": {
            "CLIENT_CLASS": "django_redis.client.DefaultClient",
            "PICKLE_VERSION": -1,  # Use the latest protocol version
            "SOCKET_TIMEOUT": 5,  # in seconds
            "COMPRESS_MIN_LEN": 10,
            "COMPRESS_COMPRESSOR": pylzma.compress,
            "COMPRESS_DECOMPRESSOR": pylzma.decompress,
            "IGNORE_EXCEPTIONS": True,
            "CONNECTION_POOL_KWARGS": {"max_connections": 100},
        }
    }
}
SESSION_ENGINE = "django.contrib.sessions.backends.file"
SESSION_CACHE_ALIAS = "default"
WEBSOCKET_URL = '/ws/'
DEBUG_TOOLBAR_PATCH_SETTINGS = False

ALLOWED_HOSTS = ['localhost', '127.0.0.1', platform.node() + '.ecmwf.int', ]
