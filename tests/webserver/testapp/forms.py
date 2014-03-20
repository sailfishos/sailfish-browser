from django import forms

class TestUploadFileForm(forms.Form):
    uploadfile = forms.FileField()
