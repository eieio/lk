LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/src/e_acos.c \
	$(LOCAL_DIR)/src/e_acosf.c \
	$(LOCAL_DIR)/src/e_acosh.c \
	$(LOCAL_DIR)/src/e_acoshf.c \
	$(LOCAL_DIR)/src/e_asin.c \
	$(LOCAL_DIR)/src/e_asinf.c \
	$(LOCAL_DIR)/src/e_atan2.c \
	$(LOCAL_DIR)/src/e_atan2f.c \
	$(LOCAL_DIR)/src/e_atanh.c \
	$(LOCAL_DIR)/src/e_atanhf.c \
	$(LOCAL_DIR)/src/e_cosh.c \
	$(LOCAL_DIR)/src/e_coshf.c \
	$(LOCAL_DIR)/src/e_exp.c \
	$(LOCAL_DIR)/src/e_expf.c \
	$(LOCAL_DIR)/src/e_fmod.c \
	$(LOCAL_DIR)/src/e_fmodf.c \
	$(LOCAL_DIR)/src/e_gamf1.c \
	$(LOCAL_DIR)/src/e_gamma.c \
	$(LOCAL_DIR)/src/e_gamma_.c \
	$(LOCAL_DIR)/src/e_gammaf.c \
	$(LOCAL_DIR)/src/e_hypot.c \
	$(LOCAL_DIR)/src/e_hypotf.c \
	$(LOCAL_DIR)/src/e_j0.c \
	$(LOCAL_DIR)/src/e_j0f.c \
	$(LOCAL_DIR)/src/e_j1.c \
	$(LOCAL_DIR)/src/e_j1f.c \
	$(LOCAL_DIR)/src/e_jn.c \
	$(LOCAL_DIR)/src/e_jnf.c \
	$(LOCAL_DIR)/src/e_lgam1.c \
	$(LOCAL_DIR)/src/e_lgam2.c \
	$(LOCAL_DIR)/src/e_lgam3.c \
	$(LOCAL_DIR)/src/e_lgamma.c \
	$(LOCAL_DIR)/src/e_log.c \
	$(LOCAL_DIR)/src/e_log10.c \
	$(LOCAL_DIR)/src/e_log10f.c \
	$(LOCAL_DIR)/src/e_logf.c \
	$(LOCAL_DIR)/src/e_pow.c \
	$(LOCAL_DIR)/src/e_powf.c \
	$(LOCAL_DIR)/src/e_rem1.c \
	$(LOCAL_DIR)/src/e_rem2.c \
	$(LOCAL_DIR)/src/e_rem_pi.c \
	$(LOCAL_DIR)/src/e_remain.c \
	$(LOCAL_DIR)/src/e_scalb.c \
	$(LOCAL_DIR)/src/e_scalbf.c \
	$(LOCAL_DIR)/src/e_sinh.c \
	$(LOCAL_DIR)/src/e_sinhf.c \
	$(LOCAL_DIR)/src/e_sqrt.c \
	$(LOCAL_DIR)/src/e_sqrtf.c \
	$(LOCAL_DIR)/src/k_cos.c \
	$(LOCAL_DIR)/src/k_cosf.c \
	$(LOCAL_DIR)/src/k_rem1.c \
	$(LOCAL_DIR)/src/k_rem_pi.c \
	$(LOCAL_DIR)/src/k_sin.c \
	$(LOCAL_DIR)/src/k_sinf.c \
	$(LOCAL_DIR)/src/k_standa.c \
	$(LOCAL_DIR)/src/k_tan.c \
	$(LOCAL_DIR)/src/k_tanf.c \
	$(LOCAL_DIR)/src/s_asinh.c \
	$(LOCAL_DIR)/src/s_asinhf.c \
	$(LOCAL_DIR)/src/s_atan.c \
	$(LOCAL_DIR)/src/s_atanf.c \
	$(LOCAL_DIR)/src/s_cbrt.c \
	$(LOCAL_DIR)/src/s_cbrtf.c \
	$(LOCAL_DIR)/src/s_ceil.c \
	$(LOCAL_DIR)/src/s_ceilf.c \
	$(LOCAL_DIR)/src/s_copy1.c \
	$(LOCAL_DIR)/src/s_copysi.c \
	$(LOCAL_DIR)/src/s_cos.c \
	$(LOCAL_DIR)/src/s_cosf.c \
	$(LOCAL_DIR)/src/s_erf.c \
	$(LOCAL_DIR)/src/s_erff.c \
	$(LOCAL_DIR)/src/s_expm1.c \
	$(LOCAL_DIR)/src/s_expm1f.c \
	$(LOCAL_DIR)/src/s_fabs.c \
	$(LOCAL_DIR)/src/s_fabsf.c \
	$(LOCAL_DIR)/src/s_fini1.c \
	$(LOCAL_DIR)/src/s_finite.c \
	$(LOCAL_DIR)/src/s_floor.c \
	$(LOCAL_DIR)/src/s_floorf.c \
	$(LOCAL_DIR)/src/s_frexp.c \
	$(LOCAL_DIR)/src/s_frexpf.c \
	$(LOCAL_DIR)/src/s_ilogb.c \
	$(LOCAL_DIR)/src/s_ilogbf.c \
	$(LOCAL_DIR)/src/s_isnan.c \
	$(LOCAL_DIR)/src/s_isnanf.c \
	$(LOCAL_DIR)/src/s_ldexp.c \
	$(LOCAL_DIR)/src/s_ldexpf.c \
	$(LOCAL_DIR)/src/s_lib_ve.c \
	$(LOCAL_DIR)/src/s_log1pf.c \
	$(LOCAL_DIR)/src/s_logb.c \
	$(LOCAL_DIR)/src/s_logbf.c \
	$(LOCAL_DIR)/src/s_mather.c \
	$(LOCAL_DIR)/src/s_modf.c \
	$(LOCAL_DIR)/src/s_modff.c \
	$(LOCAL_DIR)/src/s_next1.c \
	$(LOCAL_DIR)/src/s_nextaf.c \
	$(LOCAL_DIR)/src/s_rint.c \
	$(LOCAL_DIR)/src/s_rintf.c \
	$(LOCAL_DIR)/src/s_scal1.c \
	$(LOCAL_DIR)/src/s_scalbn.c \
	$(LOCAL_DIR)/src/s_sign1.c \
	$(LOCAL_DIR)/src/s_signga.c \
	$(LOCAL_DIR)/src/s_signif.c \
	$(LOCAL_DIR)/src/s_sin.c \
	$(LOCAL_DIR)/src/s_sinf.c \
	$(LOCAL_DIR)/src/s_tan.c \
	$(LOCAL_DIR)/src/s_tanf.c \
	$(LOCAL_DIR)/src/s_tanh.c \
	$(LOCAL_DIR)/src/s_tanhf.c \
	$(LOCAL_DIR)/src/w_acos.c \
	$(LOCAL_DIR)/src/w_acosf.c \
	$(LOCAL_DIR)/src/w_acosh.c \
	$(LOCAL_DIR)/src/w_acoshf.c \
	$(LOCAL_DIR)/src/w_asin.c \
	$(LOCAL_DIR)/src/w_asinf.c \
	$(LOCAL_DIR)/src/w_atan2.c \
	$(LOCAL_DIR)/src/w_atan2f.c \
	$(LOCAL_DIR)/src/w_atanh.c \
	$(LOCAL_DIR)/src/w_atanhf.c \
	$(LOCAL_DIR)/src/w_cabs.c \
	$(LOCAL_DIR)/src/w_cabsf.c \
	$(LOCAL_DIR)/src/w_cosh.c \
	$(LOCAL_DIR)/src/w_coshf.c \
	$(LOCAL_DIR)/src/w_drem.c \
	$(LOCAL_DIR)/src/w_dremf.c \
	$(LOCAL_DIR)/src/w_exp.c \
	$(LOCAL_DIR)/src/w_expf.c \
	$(LOCAL_DIR)/src/w_fmod.c \
	$(LOCAL_DIR)/src/w_fmodf.c \
	$(LOCAL_DIR)/src/w_gamf1.c \
	$(LOCAL_DIR)/src/w_gamma.c \
	$(LOCAL_DIR)/src/w_gamma_.c \
	$(LOCAL_DIR)/src/w_gammaf.c \
	$(LOCAL_DIR)/src/w_hypot.c \
	$(LOCAL_DIR)/src/w_hypotf.c \
	$(LOCAL_DIR)/src/w_j0.c \
	$(LOCAL_DIR)/src/w_j0f.c \
	$(LOCAL_DIR)/src/w_j1.c \
	$(LOCAL_DIR)/src/w_j1f.c \
	$(LOCAL_DIR)/src/w_jn.c \
	$(LOCAL_DIR)/src/w_jnf.c \
	$(LOCAL_DIR)/src/w_lgam1.c \
	$(LOCAL_DIR)/src/w_lgam2.c \
	$(LOCAL_DIR)/src/w_lgam3.c \
	$(LOCAL_DIR)/src/w_lgamma.c \
	$(LOCAL_DIR)/src/w_log.c \
	$(LOCAL_DIR)/src/w_log10.c \
	$(LOCAL_DIR)/src/w_log10f.c \
	$(LOCAL_DIR)/src/w_logf.c \
	$(LOCAL_DIR)/src/w_pow.c \
	$(LOCAL_DIR)/src/w_powf.c \
	$(LOCAL_DIR)/src/w_rem1.c \
	$(LOCAL_DIR)/src/w_remain.c \
	$(LOCAL_DIR)/src/w_scalb.c \
	$(LOCAL_DIR)/src/w_scalbf.c \
	$(LOCAL_DIR)/src/w_sinh.c \
	$(LOCAL_DIR)/src/w_sinhf.c \
	$(LOCAL_DIR)/src/w_sqrt.c \
	$(LOCAL_DIR)/src/w_sqrtf.c \
	$(LOCAL_DIR)/i387/e_acos.S \
	$(LOCAL_DIR)/i387/e_asin.S \
	$(LOCAL_DIR)/i387/e_atan2.S \
	$(LOCAL_DIR)/i387/e_exp.S \
	$(LOCAL_DIR)/i387/e_fmod.S \
	$(LOCAL_DIR)/i387/e_log.S \
	$(LOCAL_DIR)/i387/e_log10.S \
	$(LOCAL_DIR)/i387/e_remain.S \
	$(LOCAL_DIR)/i387/e_scalb.S \
	$(LOCAL_DIR)/i387/e_sqrt.S \
	$(LOCAL_DIR)/i387/s_atan.S \
	$(LOCAL_DIR)/i387/s_ceil.S \
	$(LOCAL_DIR)/i387/s_copysi.S \
	$(LOCAL_DIR)/i387/s_cos.S \
	$(LOCAL_DIR)/i387/s_finite.S \
	$(LOCAL_DIR)/i387/s_floor.S \
	$(LOCAL_DIR)/i387/s_ilogb.S \
	$(LOCAL_DIR)/i387/s_log1p.S \
	$(LOCAL_DIR)/i387/s_logb.S \
	$(LOCAL_DIR)/i387/s_rint.S \
	$(LOCAL_DIR)/i387/s_scalbn.S \
	$(LOCAL_DIR)/i387/s_signif.S \
	$(LOCAL_DIR)/i387/s_sin.S \
	$(LOCAL_DIR)/i387/s_tan.S \
	$(LOCAL_DIR)/i387/infinity.c \
#	$(LOCAL_DIR)/src/s_log1p.c \

include make/module.mk
