origin_test = executable('test-app-origin', 'test-app-origin.c',
    '../src/pins-app-origin.c',
    include_directories: include_directories('../src'),
    dependencies: dependency('glib-2.0'))
test('Application source classification', origin_test)
