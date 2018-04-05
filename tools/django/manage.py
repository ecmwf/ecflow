#!/usr/bin/env python
import os
import sys
import django
if __name__ == "__main__":
    os.environ.setdefault("DJANGO_SETTINGS_MODULE", "flow.settings")
    from django.core.management import execute_from_command_line        
    execute_from_command_line(sys.argv)
"""
    python manage.py supervisor --daemonize
    python manage.py supervisor stop all
    python manage.py supervisor start all

python ./manage.py runserver 0.0.0.0:8001
firefox http://localhost:8001

python manage.py runserver 1000

mysite/
    manage.py
    mysite/
        __init__.py
        settings.py
        urls.py
        wsgi.py
"""

