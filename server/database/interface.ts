export type ValueOf<O extends object> = O[keyof O];

export type LevelID = number;
export type UserIdent = string;

export type Format = 1 | 2;

export type SubmissionMetadata = {
  format: Format;
  levelid: LevelID;
  levelversion: number;
  playername: string;
  userid: string;
  userident: UserIdent;
};

export type DeathData<F extends Format> = {
  percentage: number;
  practice: 0 | 1 | boolean;
  x: number;
  y: number;
} & (F extends 2 ? {
  coins: number;
  coin1?: boolean;
  coin2?: boolean;
  coin3?: boolean;
  itemdata: number;
} : {});

export type MinDeathData = Pick<DeathData<1>, "x" | "y" | "percentage">;

export interface Database {
  list<P extends boolean>(levelId: LevelID, isPlatformer: P, inclPractice: boolean): Promise<{
    deaths: MinDeathData[P extends true ? ("x" | "y") : ("x" | "y" | "percentage")][][],
    columns: P extends true ? "x,y" : "x,y,percentage"
  }>;

  analyze<F extends Format>(levelId: LevelID, columns: string): Promise<ValueOf<DeathData<F>>[][]>;

  register<F extends Format>(metadata: Pick<SubmissionMetadata, "userident" | "levelid" | "levelversion" | "format"> & { format: F }, deaths: DeathData<F>[]): Promise<void>;
};

export async function getDriver(driver: "dummy" | "postgres"): Promise<Database> {
  const imported = await import(`./${driver}`);
  return new imported.default();
}
