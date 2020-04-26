libhhdir = $(libdir)/hildon-desktop
dathhdir = $(datadir)/applications/hildon-home
libhh_LTLIBRARIES = personal-ip-address.la
dathh_DATA = personal-ip-address.desktop

personal_ip_address_la_SOURCES = personal-ip-address.c
personal_ip_address_la_CFLAGS = $(GTK_CFLAGS) $(DESKTOP_CFLAGS) $(CONIC_CFLAGS) $(ICD2_CFLAGS) $(DBUS_GLIB_CFLAGS)
personal_ip_address_la_LIBADD = $(GTK_LIBS) $(DESKTOP_LIBS) $(CONIC_LIBS) $(ICD2_LIBS) $(DBUS_GLIB_LIBS)
personal_ip_address_la_LDFLAGS = -module -avoid-version

icondir = $(datadir)/icons/hicolor

install-data-local:
	i=personal-ip-address; \
	$(mkinstalldirs) $(DESTDIR)$(icondir)/64x64/hildon; \
	$(INSTALL_DATA) $(srcdir)/$$i-64x64.png $(DESTDIR)$(icondir)/64x64/hildon/$$i.png;

uninstall-local:
	i=personal-ip-address; \
	rm $(DESTDIR)$(icondir)/64x64/hildon/$$i.png;

EXTRA_DIST = config.rpath m4/ChangeLog 					\
        autogen.sh				\
        make-package				\
        debian/changelog			\
        debian/compat				\
        debian/copyright			\
        debian/control				\
        debian/rules				\
	debian/postinst				\
	debian/install				\
        personal-ip-address.desktop

install-data-hook:
	@(cd $(DESTDIR)$(libhhdir) && $(RM) $(libhh_LTLIBRARIES))

deb:
	./make-package $(PACKAGE) $(VERSION)

SUBDIRS = po

ACLOCAL_AMFLAGS = -I m4
