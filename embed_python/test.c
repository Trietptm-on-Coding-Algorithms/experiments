#include <stdio.h>

#include <Python.h>

int main(int ac, char **av)
{
	int rc = -1, i;
	const char *modNames[3] = {"a", "b", "c"};

	PyObject *pModule, *pFunc, *pValue, *pArgs;

	Py_SetProgramName(av[0]);
	Py_Initialize();

	for(i=0; i<3; ++i) {
		printf("on module: %s\n", modNames[i]);

		pModule = PyImport_ImportModule(modNames[i]);
		if(!pModule) {
			printf("ERROR: PyImport_ImportModule()\n");
			goto cleanup;
		}

		pFunc = PyObject_GetAttrString(pModule, "go");
		if(!pFunc) {
			printf("ERROR: PyObject_GetAttrString()\n");
			goto cleanup;
		}

		pArgs = PyTuple_New(0);
		if(!pArgs) {
			printf("ERROR: PyTuple_New()\n");
			goto cleanup;
		}

		pValue = PyObject_CallObject(pFunc, pArgs);
	}

	Py_Finalize();
	rc = 0;
	cleanup:
	return rc;
}
