require rauc.bb

SRC_URI = "git://github.com/jluebbe/rauc.git;protocol=https \
	file://dev-ca.pem \
	file://rauc-done.service \
	file://rauc-ubi.rules "

DEPENDS = "squashfs-tools-native curl-native openssl-native glib-2.0-native"

inherit native deploy
do_deploy[sstate-outputdirs] = "${DEPLOY_DIR_TOOLS}"

do_deploy() {
        install -d ${DEPLOY_DIR_TOOLS}
        install -m 0755 rauc ${DEPLOY_DIR_TOOLS}/rauc-${PV}
        rm -f ${DEPLOY_DIR_TOOLS}/rauc
        ln -sf ./rauc-${PV} ${DEPLOY_DIR_TOOLS}/rauc
	# allow override from local.conf?
	install -m 600 ${S}/test/openssl-ca/dev/autobuilder-1.cert.pem ${DEPLOY_DIR_TOOLS}/rauc.cert.pem
	install -m 600 ${S}/test/openssl-ca/dev//private/autobuilder-1.pem ${DEPLOY_DIR_TOOLS}/rauc.priv.pem
}

addtask deploy before do_package after do_install
