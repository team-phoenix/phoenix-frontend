TEMPLATE = subdirs

SUBDIRS += backend frontend

frontend.depends = backend
