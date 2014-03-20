This is a Django project that can be used to test the browser functionality
involving HTTP requests.

To run the server you have to have Django installed in your system. Or
alternatively you could use `virtualenv` command::

  $ virtualenv djangoserver
  $ workon djangoserver
  (djangoserver)$ pip install django
  (djangoserver)$ python manage.py runserver 0.0.0.0:8000

If the server starts successfully you can point your browser to it using the URL
http://<ip_address_of_your_computer>:8000/
