{
  'TOOLS': ['newlib', 'glibc', 'pnacl', 'win', 'linux'],
  'TARGETS': [
    {
      'NAME' : 'load_progress',
      'TYPE' : 'main',
      'SOURCES' : ['load_progress.cc'],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples',
  'NAME': 'load_progress',
  'TITLE': 'Load Progress',
  'GROUP': 'Concepts'
}

