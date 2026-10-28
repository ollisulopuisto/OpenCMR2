#!/usr/bin/env python3
"""Create an optional driver-name sidecar, never modify original game files."""
import argparse
from pathlib import Path
NAMES=('t. makinen','c. mcrae','r. burns','d. auriol','c. sainz',
       'j. kankkunen','m. gronholm','f. loix','f. delecour','h. rovanpera',
       't. radstrom','t. gardemeister','p. liatti','p. solberg','b. thiry')
def install(game_dir,user_dir):
    game=Path(game_dir).resolve();user=Path(user_dir).resolve()
    if not game.is_dir():raise ValueError('Original game directory is missing')
    children={p.name.lower() for p in game.iterdir() if p.is_dir()}
    if not {'game','frontend','sounds','countryspecific','setuprep'} <= children:
        raise ValueError('Choose the installed PC game directory with all five data folders')
    if user==game or game in user.parents:
        raise ValueError('Private user directory must be outside original game data')
    user.mkdir(parents=True,exist_ok=True)
    target=user/'driver-names.tsv'
    # Exclusive creation also refuses an existing symlink, without following it.
    with target.open('x',encoding='ascii',newline='\n') as output:
        output.write('OPENCMR2-DRIVER-NAMES-1\n'+''.join(f'{i}\t{name}\n' for i,name in enumerate(NAMES)))
    return target
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input-dir',type=Path,required=True)
    p.add_argument('--user-dir',type=Path,required=True)
    a=p.parse_args()
    try:print('Created',install(a.input_dir,a.user_dir),'Originals unchanged.')
    except (ValueError,OSError) as error:p.exit(1,str(error)+'\n')
