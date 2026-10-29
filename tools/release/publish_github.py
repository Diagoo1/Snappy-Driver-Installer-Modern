"""Publish the reviewed source/release using Git's existing GitHub credential.

Credentials stay in memory, are sent only to GitHub HTTPS endpoints, and are
never printed, written to disk, embedded in git remotes, or put in arguments.
"""
from pathlib import Path
import argparse,json,os,subprocess,urllib.request,urllib.error,urllib.parse,hashlib
REPOSITORY='Diagoo1/Snappy-Driver-Installer-Modern'
def credential():
    env=os.environ.copy();env.update(GIT_TERMINAL_PROMPT='0',GCM_INTERACTIVE='Never')
    result=subprocess.run(['git','credential','fill'],input=b'protocol=https\nhost=github.com\nusername=Diagoo1\n\n',capture_output=True,env=env,timeout=30)
    if result.returncode:raise RuntimeError('GitHub authentication is unavailable through the configured Git credential manager.')
    values=dict(line.split('=',1) for line in result.stdout.decode().splitlines() if '=' in line)
    token=values.get('password')
    if not token:raise RuntimeError('Git credential manager did not provide GitHub authentication.')
    return token
class Github:
    def __init__(self):self.token=credential()
    def call(self,path,method='GET',body=None,content_type='application/json'):
        url=path if path.startswith('https://') else 'https://api.github.com'+path
        if urllib.parse.urlsplit(url).hostname not in {'api.github.com','uploads.github.com'}:raise RuntimeError('Unexpected GitHub API destination.')
        data=json.dumps(body).encode() if isinstance(body,dict) else body
        request=urllib.request.Request(url,data=data,method=method,headers={'Authorization':'Bearer '+self.token,'Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28','User-Agent':'Snappy-Driver-Installer-Modern-publisher','Content-Type':content_type})
        try:
            with urllib.request.urlopen(request,timeout=180) as response:return {} if response.status==204 else json.load(response)
        except urllib.error.HTTPError as error:
            if error.code==404:return None
            # Never echo request headers or credential-manager output.
            try:message=json.load(error).get('message','')
            except (ValueError,AttributeError):message=''
            raise RuntimeError('GitHub API returned HTTP '+str(error.code)+(': '+message[:400] if message else '')) from None
    def verify(self):
        account=self.call('/user')
        if account.get('login','').lower()!='diagoo1':raise RuntimeError('The authenticated account differs from the user-authorized repository owner.')
        return {'login':account['login'],'repository':REPOSITORY,'repositoryExists':self.call('/repos/'+REPOSITORY) is not None}
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');parser.add_argument('--create-repository',action='store_true');parser.add_argument('--release');parser.add_argument('--assets',type=Path);parser.add_argument('--notes',type=Path);parser.add_argument('--commit');parser.add_argument('--prerelease',action='store_true');args=parser.parse_args()
    github=Github();state=github.verify()
    if args.check:print(json.dumps(state));return
    if args.create_repository and not state['repositoryExists']:
        repo=github.call('/user/repos','POST',{'name':REPOSITORY.split('/')[1],'description':'Modern single-executable Windows interface for the pinned Snappy Driver Installer Origin engine.','private':False,'auto_init':False,'has_issues':True,'has_wiki':False});print(json.dumps({'repository':repo['html_url']}))
    if args.release:
        if not args.assets or not args.notes or not args.commit:raise RuntimeError('Explicit reviewed assets, release notes and source commit are required.')
        version='v'+args.release
        release=github.call('/repos/'+REPOSITORY+'/releases/tags/'+version)
        if release and not release['draft']:raise RuntimeError('This public release already exists; refusing to replace its assets.')
        if release and bool(release['prerelease'])!=args.prerelease:raise RuntimeError('Existing draft has a different prerelease status.')
        if not release:release=github.call('/repos/'+REPOSITORY+'/releases','POST',{'tag_name':version,'target_commitish':args.commit,'name':version+(' Beta' if args.prerelease else ''),'body':args.notes.read_text('utf-8'),'draft':True,'prerelease':args.prerelease})
        existing={item['name']:item for item in github.call('/repos/'+REPOSITORY+'/releases/'+str(release['id'])+'/assets')}
        for file in sorted(args.assets.iterdir()):
            if not file.is_file():continue
            if file.name in existing:raise RuntimeError('Draft asset already exists: '+file.name+'; inspect before retrying.')
            url=release['upload_url'].split('{',1)[0]+'?name='+urllib.parse.quote(file.name)
            asset=github.call(url,'POST',file.read_bytes(),'application/octet-stream');expected='sha256:'+hashlib.sha256(file.read_bytes()).hexdigest()
            if asset.get('digest')!=expected or asset['size']!=file.stat().st_size:raise RuntimeError('Uploaded asset integrity mismatch.')
        final=github.call('/repos/'+REPOSITORY+'/releases/'+str(release['id']),'PATCH',{'draft':False,'make_latest':'false' if args.prerelease else 'true'})
        print(json.dumps({'release':final['html_url'],'assets':[{ 'name':item['name'],'digest':item.get('digest')} for item in github.call('/repos/'+REPOSITORY+'/releases/'+str(release['id'])+'/assets')]}))
if __name__=='__main__':main()
