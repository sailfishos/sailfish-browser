# Create your views here.
from django.http import HttpResponse
from django.http import HttpResponseRedirect
from base64 import b64decode
from django.shortcuts import render
from forms import TestUploadFileForm

def home(request):
    return render(request, 'home.html')

def testauth(request):

    print "in testauth() veiw"
    print request
    if 'HTTP_AUTHORIZATION' in request.META:
        _, hashauth = request.META['HTTP_AUTHORIZATION'].split(" ")
        if b64decode(hashauth) == "user:password":
            return HttpResponse("Welcome")
    resp = HttpResponse("Authentication required", status=401)
    resp['WWW-Authenticate'] = 'Basic realm="Protected Area"'
    return resp

def testupload(request):
    if request.method == 'POST':
        form = TestUploadFileForm(request.POST, request.FILES)
        print request.FILES
        if form.is_valid():
            return HttpResponseRedirect('/upload')
    else:
        form = TestUploadFileForm()
    return render(request, 'upload.html', {'form': form})

def show_ajax_page(request):
    return render(request, 'ajax_page.html')

def testajaxreq(request):
    return HttpResponse(content="Success!")
