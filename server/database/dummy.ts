import { Database } from "./interface";

export default class implements Database {

  async list<P extends boolean>(_levelId: any, isPlatformer: P, _inclPractice: boolean) {
    // @ts-ignore
    const columns: P extends true ? "x,y" : "x,y,percentage" = isPlatformer ? "x,y" : "x,y,percentage";
    return { deaths: [], columns };
  }

  async analyze(_levelId: any, _columns: any) {
    return [];
  }

  async register(metadata: any, deaths: any) {
    console.log(metadata, deaths);
  }

}
