# needs waf SVN with my gsettings support patch, hopefully soon to be merged

VERSION = '0.0.1'
APPNAME = 'gsettings-test'

top = '.'
out = 'build'

def options(opt):
	opt.load ('compiler_cc glib2 gnu_dirs')
	opt.parser.remove_option ('--oldincludedir')
	opt.parser.remove_option ('--htmldir')
	opt.parser.remove_option ('--dvidir')
	opt.parser.remove_option ('--pdfdir')
	opt.parser.remove_option ('--psdir')

def configure(conf):
	conf.load ('compiler_cc vala gnu_dirs glib2')

	conf.check_cfg (package='glib-2.0', uselib_store='GLIB', atleast_version='2.25.0', mandatory=1,
	                args='--cflags --libs')
	conf.check_cfg (package='gio-2.0', uselib_store='GIO', atleast_version='2.24.0', mandatory=1,
	                args='--cflags --libs')
	conf.check_cfg (package='gtk+-2.0', uselib_store='GTK', atleast_version='2.20.0', mandatory=1,
	                args='--cflags --libs')

	#conf.env['VALAFLAGS'] = '-g'
	conf.env['CCFLAGS'] = '-g'

	# For SHDeleteKey, which is much more use than RegDeleteKey
	conf.env['LINKFLAGS'] = '-lshlwapi'

def build(bld):
	app = bld (features = 'c cprogram glib2',
	           packages = 'gtk+-2.0',
	           target = 'gsettings-test',
	           uselib = 'GTK GLIB GIO',
	           settings_schema_files = ['schemas/org.gsettings.test.gschema.xml'],
	           source = ['src/main.vala'])

	# FIXME: enums namespace file gets generated twice, which causes some errors ..
	# also the schema and enums files get installed and uninstalled twice ..

	storage_test = bld (features = 'c cprogram glib2',
	                    packages = 'gtk+-2.0',
	                    target = 'storage-test',
	                    uselib = 'GTK GLIB GIO',
	                    settings_enum_namespace = 'org.gsettings.test',
	                    settings_enum_files = ['src/storage-test-enums.h'],
	                    settings_schema_files = ['schemas/org.gsettings.test.storage-test.gschema.xml'],
	                    source = ['src/storage-test.c'])

	notify_test = bld (features = 'c cprogram glib2',
	                   packages = 'gtk+-2.0',
	                   target = 'notify-test',
	                   uselib = 'GTK GLIB GIO',
	                   source = ['src/notify-test.c'])

	spped_test = bld (features = 'c cprogram glib2',
	                  packages = 'gtk+-2.0',
	                  target = 'speed-test',
	                  uselib = 'GTK GLIB GIO',
	                  source = ['src/speed-test.c'])
