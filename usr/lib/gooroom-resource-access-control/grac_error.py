#! /usr/bin/env python3

#-----------------------------------------------------------------------
class GracError(Exception):
    """
    Grac Custiom Exception
    """

    def __init__(self,*args,**kwargs):
        Exception.__init__(self,*args,**kwargs)

#-----------------------------------------------------------------------
class GracGoto(Exception):
    """
    Grac goto
    """

    def __init__(self,*args,**kwargs):
        Exception.__init__(self,*args,**kwargs)
