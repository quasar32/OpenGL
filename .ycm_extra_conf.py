def Settings( **kwargs ):
  return {
    'flags': [ 
        '-x', 
        'c', 
        '-Iglad/include',
        '-Icglm/include'
    ],
  }
