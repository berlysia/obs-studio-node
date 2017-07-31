import Vue from 'vue';
import { Subject } from 'rxjs/Subject';
import { Subscription } from 'rxjs/Subscription';
import { mutation, StatefulService, InitAfter, Inject, Mutator } from './stateful-service';
import { SourcesService, ISource, Source } from './sources';
import { ScenesService } from './scenes';
import { nodeObs } from './obs-api';
import Utils from './utils';


const VOLMETER_UPDATE_INTERVAL = 40;

export interface IAudioSource {
  sourceId: string;
  fader: IFader;
}

export interface IVolmeter {
  level: number;
  magnitude: number;
  peak: number;
  muted: number;
}

interface IFader {
  db: number;
  deflection: number;
}

interface IAudioSourcesState {
  audioSources: Dictionary<IAudioSource>;
}

interface INodeObsId {
  id: number;
}



@InitAfter(SourcesService)
export class AudioService extends StatefulService<IAudioSourcesState> {

  static initialState: IAudioSourcesState = {
    audioSources: {}
  };

  @Inject() private sourcesService: SourcesService;
  @Inject() private scenesService: ScenesService;


  protected mounted() {

    this.scenesService.sourceAdded.subscribe(sceneSourceModel => {
      const source = this.sourcesService.getSource(sceneSourceModel.sourceId);
      if (!source.audio) return;
      this.createAudioSource(source);
    });

    this.sourcesService.sourceUpdated.subscribe(source => {
      const audioSource = this.getSource(source.sourceId);
      if (!audioSource) return;

      if (!source.audio) {
        this.REMOVE_AUDIO_SOURCE(source.sourceId);
        return;
      }

    });

    this.sourcesService.sourceRemoved.subscribe(source => {
      if (source.audio) this.REMOVE_AUDIO_SOURCE(source.sourceId);
    });

  }

  getSource(sourceId: string): AudioSource {
    return this.state.audioSources[sourceId] ? new AudioSource(sourceId) : void 0;
  }

  getSourcesForCurrentScene(): AudioSource[] {
    const scene = this.scenesService.activeScene;
    const sceneSources = scene.getItems({ showHidden: true }).filter(source => source.audio);
    return sceneSources.map((sceneSource: ISource) => this.getSource(sceneSource.sourceId)).filter(item => item);
  }


  fetchAudioSource(sourceName: string): IAudioSource {
    const source = this.sourcesService.getSourceByName(sourceName);
    const fader = nodeObs.OBS_content_getSourceFader(sourceName);
    fader.db = fader.db || 0;
    return {
      sourceId: source.sourceId,
      fader
    };
  }


  private createAudioSource(source: Source) {
    const volmeter = nodeObs.OBS_audio_createVolmeter(2);
    const fader = nodeObs.OBS_audio_createFader(0);

    nodeObs.OBS_audio_volmeterAttachSource(volmeter, source.name);
    nodeObs.OBS_audio_faderAttachSource(fader, source.name);

    this.ADD_AUDIO_SOURCE(this.fetchAudioSource(source.name));
  }


  @mutation()
  private ADD_AUDIO_SOURCE(source: IAudioSource) {
    Vue.set(this.state.audioSources, source.sourceId, source);
  }


  @mutation()
  private REMOVE_AUDIO_SOURCE(sourceId: string) {
    Vue.delete(this.state.audioSources, sourceId);
  }
}

@Mutator()
export class AudioSource extends Source implements IAudioSource {
  fader: IFader;

  @Inject()
  private audioService: AudioService;

  private audioSourceState: IAudioSource;

  constructor(sourceId: string) {
    super(sourceId);
    this.audioSourceState = this.audioService.state.audioSources[sourceId];
    Utils.applyProxy(this, this.audioSourceState);
  }


  setDeflection(deflection: number) {
    const fader = nodeObs.OBS_content_getSourceFader(this.name);
    nodeObs.OBS_audio_faderSetDeflection(fader, deflection);
    this.UPDATE(this.audioService.fetchAudioSource(this.name));
  }

  setMuted(muted: boolean) {
    this.sourcesService.setMuted(this.sourceId, muted);
  }


  subscribeVolmeter(cb: (volmeter: IVolmeter) => void): Subscription {
    const volmeterStream = new Subject<IVolmeter>();
    const volmeterId = nodeObs.OBS_content_getSourceVolmeter(this.name) as INodeObsId;

    // TODO: calling this function causes a crash in ava tests
    // https://github.com/twitchalerts/node-obs/issues/156
    // the default interval is 40ms
    // nodeObs.OBS_audio_volmeterSetUpdateInterval(volmeterId, VOLMETER_UPDATE_INTERVAL);

    let gotEvent = false;
    let lastVolmeterValue: IVolmeter;
    let volmeterCheckTimeoutId: number;
    const obsSubscription: INodeObsId = nodeObs.OBS_audio_volmeterAddCallback(volmeterId, (volmeter: IVolmeter) => {
      volmeterStream.next(volmeter);
      lastVolmeterValue = volmeter;
      gotEvent = true;
    });

    function volmeterCheck() {
      if (!gotEvent) {
        volmeterStream.next({ ...lastVolmeterValue, level: 0, peak: 0 });
      }

      gotEvent = false;
      volmeterCheckTimeoutId = setTimeout(volmeterCheck, VOLMETER_UPDATE_INTERVAL * 2);
    }

    volmeterCheck();

    return volmeterStream.subscribe(cb).add(() => {
      clearTimeout(volmeterCheckTimeoutId);
      nodeObs.OBS_audio_volmeterRemoveCallback(volmeterId, obsSubscription);
    });
  }


  @mutation()
  private UPDATE(patch: { sourceId: string } & Partial<IAudioSource>) {
    Object.assign(this.audioSourceState, patch);
  }

}