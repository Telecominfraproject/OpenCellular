# from opentest.server.service.tftp_service import ArtifactoryTFTPServiceClient
from nute import utils
import tempfile
import subprocess
import re
import os
import shutil

from view_utils import append_unit_info

# class LTETFTPServiceClient(ArtifactoryTFTPServiceClient):
#
#     def create_service(self, context, retry = False):
#         localip = context.STATION_LOCALIP
#
#         artifactory_config = context.get_as_dict('ARTIFACTORY')
#         artifactory_url = artifactory_config['SERVER_URL']
#
#         if context.FLASH_VERSION_DESIRED is not None:
#             image_path = 'oc-lte/' + context.FLASH_VERSION_DESIRED
#             append_unit_info(context, 'Artifactory Image: %s (not AUTO)' % (image_path))
#         else:
#             image_paths_based_on_fe_rev = artifactory_config['SOFTWARE_PATH_FROM_FE_REV']
#             #image_paths_based_on_fe_rev is a dict containing:
#             # {None: '<default path>', 'D': 'Rev D path', 'E': 'Rev E path'}
#             fe_rev = getattr(context, 'FE_REV', None)
#             image_path = image_paths_based_on_fe_rev[fe_rev]
#             append_unit_info(context, 'Artifactory Image: %s (FE Rev[%s])' % (image_path, fe_rev))
#
#         tftp_folder = super(LTETFTPServiceClient, self).create_service(localip, artifactory_url, image_path, offline_allowed=True)
#         context.UPDATEFINAL_FILE_LOCAL_PATH = tftp_folder
#
        # tftp_file = os.path.join(tftp_folder, 'tftpenv.txt')
        # if os.path.exists(tftp_file):
        #     context.logger.info('Using TFTP env file from artifact')
        #     context.UBOOT_SETUPFILEENV = tftp_file
        #
        # finalenv_file = os.path.join(tftp_folder, 'e2eenv.txt')
        # if os.path.exists(finalenv_file):
        #     context.logger.info('Using E2E env file from artifact')
        #     context.UBOOT_FINALFILEENV = finalenv_file

def choose_env_files(context, tftp_folder):
    tftp_file = os.path.join(tftp_folder, 'tftpenv.txt')
    if os.path.exists(tftp_file):
        context.logger.info('Using TFTP env file from artifact')
        context.UBOOT_SETUPFILEENV = tftp_file

    finalenv_file = os.path.join(tftp_folder, 'e2eenv.txt')
    if os.path.exists(finalenv_file):
        context.logger.info('Using E2E env file from artifact')
        context.UBOOT_FINALFILEENV = finalenv_file



def update_software(context):
    src_folder = context.UPDATEFINAL_FILE_LOCAL_PATH
    UPDATE_ENTRIES = [
    {'file': 'lsm_os.gz', 'timeout': 200},
    {'file': 'lsm_rd.gz', 'timeout': 1200}
    ]

    ignore_files = set()
    for update_entry in UPDATE_ENTRIES:
        context.logger.info('Updating ' + update_entry['file'] + ', timeout is %d' % update_entry['timeout'])
        context.server.scp.putfile(update_entry['file'], src_folder, '/tmp/')
        context.server.ssh.execute_command(
            'update -o /tmp/%s; update -b /tmp/%s' % (update_entry['file'], update_entry['file']),
            timeout=update_entry['timeout'])
        ignore_files.add(update_entry['file'])

    uboot_filename = 'u-boot-octeon_tip.bin'
    context.logger.info('Updating u-boot')
    context.server.scp.putfile(uboot_filename, src_folder, '/tmp/')
    context.server.ssh.execute_command('update-uboot -n /tmp/' + uboot_filename, timeout=50)
    ignore_files.add(uboot_filename)

    # Transfer all other files to /mnt/app and ignore previous files
    for root, dirs, files in os.walk(src_folder):
        for f in files:
            if f not in ignore_files:
                context.logger.info('Transfering ' + f  + ' to mnt/app/')
                context.server.scp.putfile(f, src_folder, '/mnt/app/')
