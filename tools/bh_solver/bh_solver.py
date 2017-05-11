import ctypes
import numpy as np
from scipy.optimize import basinhopping
from timeit import default_timer as perftimer
import sys


class NloptFunctor:
    def __init__(self, dim, func):
        self._dim = dim
        self._func = func
        self._func.restype = ctypes.c_double

    def __call__(self, ndarr):
        return self._func(self._dim, ndarr.ctypes.data_as(ctypes.POINTER(ctypes.c_double)), None, None)


class BHConfig:
    def __init__(self):
        self.Timeout = 600
        self.MaxIter = 100
        self.MaxLocalIter = 100000
        self.RelTolerance = 1e-8
        self.Temperature = 1.0
        self.StepSize = 0.5
        self.Interval = 50

    def __str__(self):
        res = "TO:" + str(self.Timeout) + "," \
              + "GITER:" + str(self.MaxIter) + "," \
              + "LITER:" + str(self.MaxLocalIter) + "," \
              + "FTOL:" + str(self.RelTolerance) + "," \
              + "TEMP:" + str(self.Temperature) + "," \
              + "STEP:" + str(self.StepSize) + "," \
              + "IVAl:" + str(self.Interval)
        return res


bh_cfg = BHConfig()
#print(bh_cfg)


def basinhopping_t(func, x_arr):
    import signal

    def handler(signum, frame):
        raise TimeoutError()

    signal.signal(signal.SIGALRM, handler)
    # default timeout in seconds
    signal.alarm(bh_cfg.Timeout)
    try:
        nperror = 'ignore'
        with np.errstate(divide=nperror, over=nperror, invalid=nperror):
            # You can try to play with options of local minimizer. Doing that in Scipy 0.18 can result in slowdown
            # options = {'maxiter': bh_cfg.MaxLocalIter, 'ftol': bh_cfg.RelTolerance}
            # minimizer_kwargs = {"method": "Powell", "options": options}

            minimizer_kwargs = {"method": "Powell"}
            result = basinhopping(func, x_arr, niter=bh_cfg.MaxIter, T=bh_cfg.Temperature, stepsize=bh_cfg.StepSize,
                                  minimizer_kwargs=minimizer_kwargs,
                                  interval=bh_cfg.Interval)
            status = ''
    except TimeoutError as to:
        result = None
        status = 'timeout'
    except RuntimeError as rt:
        result = None
        status = 'error'
    finally:
        signal.alarm(0)
    return result, status

def elapsed_time(end_time, start_time):
    return "%0.3f" % (end_time - start_time)


def main():
    if len(sys.argv) == 1:
        sys.stderr.write("Please provide path to folder where libgofuncs exists!!\n")
        sys.exit()

    funcs_lib_path = sys.argv[1]
    funcs_api = funcs_lib_path + '/gofuncs.api'
    funcs_lib = funcs_lib_path + '/libgofuncs.so'

    gofuncs_lib = ctypes.cdll.LoadLibrary(funcs_lib)
    # load api functions
    func_name_dims = [line.rstrip('\n').split(',') for line in open(funcs_api)]
    # get handles to functions and set attributes
    func_handles = []
    for i in range(len(func_name_dims)):
        func_handles.append(NloptFunctor(int(func_name_dims[i][1]), getattr(gofuncs_lib, func_name_dims[i][0])))

    for i in range(len(func_handles)):
        # set up initial guess vector
        x_arr = np.zeros(int(func_name_dims[i][1]))
        start_time = perftimer()
        res = basinhopping_t(func_handles[i], x_arr)
        if res[0] is None:
            end_time = perftimer()
            print(func_name_dims[i][0] + "," + res[1] + ","
                  + elapsed_time(end_time, start_time) + "," + "INF")
            continue
        if res[0].fun == 0:
            end_time = perftimer()
            status = 'sat'
            print(func_name_dims[i][0] + "," + status + ","
                  + elapsed_time(end_time, start_time) + "," + str(res[0].fun))
            # print satisfying model
            # print(str(res[0].x))
            continue
        end_time = perftimer()
        status = 'unsat'
        print(func_name_dims[i][0] + "," + status + ","
              + elapsed_time(end_time, start_time) + "," + str(res[0].fun))


if __name__ == '__main__':
    main()
