import Task,Utils
from TaskGen import taskgen,before,after,feature

# Essentially a waf port of the gsettings.m4 macro:
#   http://git.gnome.org/browse/glib/tree/m4macros/gsettings.m4

def process_schemas(self):

	# 1. process gsettings_enum_files (generate .enums.xml)
	#
	enums_tgt_node = []
	if hasattr(self, 'gsettings_enum_files'):
		enums_task = self.create_task('glib_mkenums_hack')

		source_list = self.to_list(self.gsettings_enum_files)
		source_list = [self.path.find_resource(k)for k in source_list]
		enums_task.set_inputs(source_list)
		enums_task.env['GLIB_MKENUMS_SOURCE'] = [k.abspath(enums_task.env)for k in source_list]

		target = self.gsettings_enum_namespace + '.enums.xml'
		tgt_node = self.path.find_or_declare(target)
		enums_task.set_outputs(tgt_node)
		enums_task.env['GLIB_MKENUMS_TARGET'] = tgt_node.abspath(enums_task.env)
		enums_tgt_node = [tgt_node]

		options = '--comments "<!-- @comment@ -->" --fhead "<schemalist>" --vhead "  <@type@ id=\\"%s.@EnumName@\\">" --vprod "    <value nick=\\"@valuenick@\\" value=\\"@valuenum@\\"/>" --vtail "  </@type@>" --ftail "</schemalist>" ' % (self.gsettings_enum_namespace)
		enums_task.env['GLIB_MKENUMS_OPTIONS'] = options


	# 2. process gsettings_schema_files (validate .gschema.xml files)
	#
	for schema in getattr(self,'gsettings_schema_files',[]):
		schema_task = self.create_task ('glib_validate_schema')

		schema_node = self.path.find_resource(schema)
		if not schema_node:
			raise Utils.WafError("Cannot find the schema file '%s'" % schema)
		source_list = enums_tgt_node + [schema_node]

		schema_task.set_inputs (source_list)
		schema_task.env['GLIB_COMPILE_SCHEMAS_OPTIONS'] = [("--schema-file=" + k.abspath(schema_task.env))for k in source_list]

		target_node = schema_node.change_ext ('.xml.valid')
		schema_task.set_outputs (target_node)
		schema_task.env['GLIB_VALIDATE_SCHEMA_OUTPUT'] = target_node.abspath(schema_task.env)

	# 3. schemas install task
	def compile_schemas(bld):
		if not bld.is_install: return
		Utils.pprint('YELLOW','Updating GSettings schema cache')
		command=Utils.subst_vars("${GLIB_COMPILE_SCHEMAS} ${GSETTINGSSCHEMADIR}", bld.env)
		ret=Utils.exec_command(command)

	if self.bld.is_install:
		self.bld.install_files (self.env['GSETTINGSSCHEMADIR'], schema_task.inputs)
		if not hasattr(self.bld, '_compile_schemas_registered'):
			self.bld.add_post_fun (compile_schemas)
			self.bld._compile_schemas_registered = True

def gsettings_set_options(opt):
	# FIXME: put in correct option group
	opt.add_option ('--gsettingsschemadir', help='GSettings schema location [Default: ${datadir}/glib-2.0/schemas]',default='',dest='GSETTINGSSCHEMADIR')
	opt.add_option ('--gsettingsschemadir', help='GSettings schema location [Default: ${datadir}/glib-2.0/schemas]',default='',dest='GSETTINGSSCHEMADIR')


import os
def gsettings_detect(conf):
	# GSettings: this stuff should be built into waf eventually
	# FIXME: when cross-compiling, gsettings.m4 locates the program with the following:
	#   pkg-config --variable glib_compile_schemas gio-2.0
	glib_compile_schemas=conf.find_program('glib-compile-schemas',var='GLIB_COMPILE_SCHEMAS')

	def getstr(varname):
		return getattr(Options.options,varname,getattr(conf.env,varname,''))
	prefix = conf.env['PREFIX']
	gsettingsschemadir = getstr('GSETTINGSSCHEMADIR')
	if not gsettingsschemadir:
		datadir = getstr('DATADIR')
		print datadir, 'or', conf.env['DATADIR']
		if not datadir:datadir=os.path.join(prefix,'share')
		gsettingsschemadir = os.path.join(datadir, 'glib-2.0', 'schemas')
	conf.define('GSETTINGSSCHEMADIR', gsettingsschemadir)

# FIXME: hack for win32
import Options
if Options.platform=='win32':
	# FIXME: before='c cxx' is not right, it should be before schemas are validated
	Task.simple_task_type('glib_mkenums_hack','c:/tools/mingw/bin/perl.exe ${GLIB_MKENUMS} ${GLIB_MKENUMS_OPTIONS} ${GLIB_MKENUMS_SOURCE} > ${GLIB_MKENUMS_TARGET}',color='PINK',before='glib_validate_schema')
else:
	Task.simple_task_type('glib_mkenums_hack','${GLIB_MKENUMS} ${GLIB_MKENUMS_OPTIONS} ${GLIB_MKENUMS_SOURCE} > ${GLIB_MKENUMS_TARGET}',color='PINK',before='glib_validate_schema')

Task.simple_task_type ('glib_validate_schema', 'rm -f ${GLIB_VALIDATE_SCHEMA_OUTPUT} && ${GLIB_COMPILE_SCHEMAS} --dry-run ${GLIB_COMPILE_SCHEMAS_OPTIONS} && touch ${GLIB_VALIDATE_SCHEMA_OUTPUT}',color='PINK')


#before('apply_core')(process_schemas)

# Option 1:
#  - app with schemas must set gsettings feature
#  - must set gsettings_schema_files and optionally gsettings_enum_namespace and gsettings_enum_files
feature ('gsettings') (process_schemas)


# for waf 1.5.18

VERSION = '0.0.1'
APPNAME = 'gsettings-test'

def set_options(opt):
	opt.tool_options ('compiler_cc')

	opt.tool_options ('gnu_dirs')
	opt.parser.remove_option ('--oldincludedir')
	opt.parser.remove_option ('--htmldir')
	opt.parser.remove_option ('--dvidir')
	opt.parser.remove_option ('--pdfdir')
	opt.parser.remove_option ('--psdir')

	#opt.tool_options ('glib2')
	gsettings_set_options (opt)

def configure(conf):
	conf.check_tool ('compiler_cc vala gnu_dirs glib2')

	conf.check_cfg (package='glib-2.0', uselib_store='GLIB', atleast_version='2.24.0', mandatory=1,
	                args='--cflags --libs')
	conf.check_cfg (package='gio-2.0', uselib_store='GIO', atleast_version='2.24.0', mandatory=1,
	                args='--cflags --libs')
	conf.check_cfg (package='gtk+-2.0', uselib_store='GTK', atleast_version='2.20.0', mandatory=1,
	                args='--cflags --libs')

	conf.env['VALAFLAGS'] = '-g'
	conf.env['CCFLAGS'] = '-g'

	# For SHDeleteKey, which is much more use than RegDeleteKey
	conf.env['LINKFLAGS'] = '-lshlwapi'

	gsettings_detect (conf)

def build(bld):
	app = bld (features = 'cc cprogram gsettings',
	           packages = 'gtk+-2.0',
	           target = 'gsettings-test',
	           uselib = 'GTK GLIB GIO',
	           gsettings_schema_files = ['schemas/org.gsettings.test.gschema.xml'],
	           source = ['src/main.vala'])
	#app.find_sources_in_dirs (['src', 'schemas'])

	# FIXME: enums namespace file gets generated twice, which causes some errors ..
	# also the schema and enums files get installed and uninstalled twice ..

	storage_test = bld (features = 'cc cprogram gsettings',
	                    packages = 'gtk+-2.0',
	                    target = 'storage-test',
	                    uselib = 'GTK GLIB GIO',
	                    gsettings_enum_namespace = 'org.gsettings.test',
	                    gsettings_enum_files = ['src/storage-test-enums.h'],
	                    gsettings_schema_files = ['schemas/org.gsettings.test.storage-test.gschema.xml'],
	                    source = ['src/storage-test.c'])

	notify_test = bld (features = 'cc cprogram gsettings',
	                   packages = 'gtk+-2.0',
	                   target = 'notify-test',
	                   uselib = 'GTK GLIB GIO',
	                   source = ['src/notify-test.c'])

	spped_test = bld (features = 'cc cprogram gsettings',
	                  packages = 'gtk+-2.0',
	                  target = 'speed-test',
	                  uselib = 'GTK GLIB GIO',
	                  source = ['src/speed-test.c'])

## OPTIONS FOR SPECIFYING THE SETTINGS FILE

## 1   (like marshals and enums)
#	app = bld (...)
#	app.add_gschema_files ('test/*.gschema.xml')
#	app.add_gschema_enum_files ('src/*.h', 'org.gsettings.test')


## 2   (more implicit)
#	app = bld (...,#
#	           gsettings_enum_namespace = 'org.gsettings.test')
#	app.find_sources_in_dirs(['src', 'data'])


# alternate
#	app = bld (...,
#	           gsettings_enum_namespace = 'org.gsettings.test',
#	           gsettings_enum_files = ['src/enums.h'],
#	           gsettings_schema_files = ['data/org.gsettings.test.gschema.xml'])



## 3   (functions)
#	app = bld (...)
#	check_schema_files ('data/*.gschema.xml')
#	
#	install:
#		install_schema_files ()

#	clean:
#		remove_schema_files ()

